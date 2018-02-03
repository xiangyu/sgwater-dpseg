#include "utils.h"
#include "Utterance.h"
#include "State.h"
#include "Urn.h"

int Utterance::_init = -1;
extern Count debug_level;

void
Utterance::set_init(string b_init) {
  if (b_init == "utt")
    _init = UTT_INIT;
  else if (b_init == "pho")
    _init = PHO_INIT;
  else if (b_init == "ran")
    _init = RAN_INIT;
  else if ((b_init == "true") || (b_init == "True"))
    _init = TRUE_INIT;
  else {
    cerr << "Unknown utterance initialization: " << b_init << endl;
    exit(0);
  }
}

//p_segment is prob of a point being a wd boundary
// in random segmentation.
Utterance::Utterance(const string& reference, Float p_segment)
  :_reference(reference), _score(-1)
{
  assert((p_segment >= 0) && (p_segment < 1));
  my_assert((_init >= RAN_INIT) && (_init <= TRUE_INIT), _init);
  string word;
  // remove SENTINEL character to create _unsegmented
  // and create list of words for _reference_words
  for (string::const_iterator iter = _reference.begin();
       iter != _reference.end(); iter++) {
    if (*iter == SENTINEL) { //end of reference word
      my_assert(!word.empty(), _reference);
      _reference_words.push_back(word);
      word.clear();
      if (_init == TRUE_INIT) {
	_boundaries.back() = Boundary(true);
      }
    }
    else {
      word += *iter;
      _unsegmented += *iter;
      if (_init == TRUE_INIT) {
	_boundaries.push_back(Boundary(false));
      }
      else if (_init == UTT_INIT) {     //initialize with no boundaries
	_boundaries.push_back(Boundary(false));
      }
      else if (_init == PHO_INIT) {  //initialize with all boundaries
	_boundaries.push_back(Boundary(true));
      }
      else { //add random SENTINEL chars
	double val = double(rand())/RAND_MAX;
	if (val < p_segment) {
	  _boundaries.push_back(Boundary(true));
	}
	else {
	  _boundaries.push_back(Boundary(false));
	}
      }
    }
  }
  _boundaries.back() = Boundary(true, true); // change final position b/c always a boundary
  assert(_boundaries.size() == _unsegmented.length());
}

void 
Utterance::add_counts_to_lex(Lexicon& word_counts, BiLexicon& bg_counts, Count model) {
  Count beg = 0;
  Count pos = 0;
  string prev(U_EDGE);
  string curr;
  foreach(Boundaries, b, _boundaries) {
    if (b->yes) {
      curr = _unsegmented.substr(beg, pos-beg+1);
      word_counts.inc(curr);
      if (model > 1)
	b->table = bg_counts.inc(Bigram(prev,curr));
      beg = pos+1;
      prev = curr;
    }
    pos++;
  }
  if (model > 1)
    _boundaries.back().final_table = bg_counts.inc(Bigram(prev,U_EDGE));
}

//builds a string with SENTINEL at boundary pts.
string
Utterance::get_segmented() const {
  string segm;
  Count beg = 0;
  Count pos = 0;
  cforeach(Boundaries, b, _boundaries) {
    if (b->yes) {
      segm += _unsegmented.substr(beg, pos-beg+1);
      segm += SENTINEL;
      beg = pos+1;
    }
    pos++;
  }
  return segm;
}


//builds a list of words
Utterance::Words
Utterance::get_segmented_words() {
  Words words;
  Count beg = 0;
  Count pos = 0;
  string word;
  cforeach(Boundaries, b, _boundaries) {
    if (b->yes) {
      word = _unsegmented.substr(beg, pos-beg+1);
      my_assert(!word.empty(), _unsegmented);
      words.push_back(word);
      //      cout << word << endl;
      word.clear();
      beg = pos+1;
    }
    pos++;
  }
  return words;
}

// use annealing temperature
void 
Utterance::sample(State& state, Float temp, Count model) {
  if (_unsegmented.size() == 1) 
    return;
  if (model == 2) {
  for (Count i = 0; i < _boundaries.size()-1; i++) {
    sample_bigram(i,state,temp);
  }
  }
  else { 
  //sample single boundaries
  // final boundary posn must always be true, so don't sample it.
  for (Count i = 0; i < _boundaries.size()-1; i++) {
    sample_one(i,state,temp);
  }
  }
}

/*log posterior in unigram model
nutts is number of utts seen so far.
lexicon is words seen so far.
We use the static functions so that we can specify
the lexicon used in the calculation.
*/
Float
Utterance::log_posterior(Count nutts, Lexicon& lexicon, const State& state) const {
  debug_output(800, "Utterance::log_posterior:\n", *this);
  string wd;
  Float prob = 0; //log prob
   for (Count i = 0; i < _boundaries.size(); i++) {
    wd += _unsegmented[i];
    if (_boundaries[i].yes) {
      Float p_cont = State::p_cont2(lexicon.ntokens(), nutts);
      Float p;
      // S -> W S
      if (i < _boundaries.size() - 1)
	p = p_cont;
      //S -> W
      else
	p = 1-p_cont;
      // W -> x_1 .. x_n
      //calcs for non-unique lexicon
      Float q = (lexicon(wd) + State::p_word(wd))/
	(lexicon.ntokens() + State::alpha());
      typedef pair<string, FF> SFF;
      debug_output(800, "Utterance::log_posterior() (w, (p(w), p(utt_b))) = ", SFF(wd,FF(q,p)));
      prob += log(p*q);
      debug_output(800, "Utterance::log_posterior() (w, log(p(w)*p(utt_b))) = ", SF(wd,log(p*q)));
      /* //calcs for unique lexicon
      if (lexicon(wd))
	prob += log(lexicon(wd)/(lexicon.ntokens() + State::alpha()));
      else
	prob += log(State::p_word(wd)/(lexicon.ntokens() + State::alpha()));
      */
      lexicon.inc(wd);
      wd = "";
    }
  }
  return prob;
}

/*log posterior in bigram model
nutts is number of utts seen so far.
lexicon is words seen so far.
bilex is bigrams seen so far.
We use the static functions so that we can specify
the lexicon used in the calculation.
Note that this returns the probability of the current
*segmentation*, summing over assignments to bigram tables.
*/
Float
Utterance::log_posterior(Count nutts, Lexicon& lexicon, BiLexicon& bilex, const State& state) const {
  debug_output(800, "Utterance::log_posterior:\n", *this);
  string wd;
  string prev(U_EDGE); // previous word
  Count prev_count = nutts; // count of previous word.  At start of utt, equals number of $$.
  Float prob = 0; //log prob
   for (Count i = 0; i < _boundaries.size(); i++) {
    wd += _unsegmented[i];
    if (_boundaries[i].yes) {
      // S_ij -> W_jk S_jk
      Bigram bg(prev,wd);
      Count table = _boundaries[i].table;
      prob += log(joint_predictive_dist(bg, prev_count, bilex, table));
      debug_output(800, "Utterance::log_posterior() log_p = ", prob);
      // context count for next word should not include this instance,
      //so get count before incrementing unigram stats.
      prev_count = lexicon(wd); 
      lexicon.inc(wd);
      bilex.place(bg, table, 1); //use "unsafe" mode to allow table placement out of order
      prev = wd;
      wd = "";
    }
  }
  //S_jk -> $
   Bigram bg(prev,U_EDGE);
   Count table = _boundaries.back().final_table;
   prob += log(joint_predictive_dist(bg, prev_count, bilex, table));
   debug_output(800, "Utterance::log_posterior() log_p = ", prob);
   bilex.place(bg, table, 1); //use "unsafe" mode to allow table placement out of order
   return prob;
}

//samples a single boundary point at position i
//with temperature temp.
void
Utterance::sample_one(Count i, State& state, Float temp) {
  Lexicon& lexicon = state.get_lexicon();   
  string left = left_word(i);  
  string right = right_word(i);
  string center = center_word(i);
  if (_boundaries[i].yes) {
    lexicon.dec(left);
    lexicon.dec(right);
  }
  else {
    lexicon.dec(center);
  }
  Float denom = (lexicon.ntokens()+ state.alpha());
  Float yes = state.p_cont() * 
    numer_base(left,state) * //denom cancels w/ no case
    (numer_base(right,state) + kdelta(left,right)) / (denom+1);
  Float no = numer_base(center,state); //denom cancels w/ yes case
#ifndef NDEBUG
  if (debug_level >= 550) cout << "p_cont: " << state.p_cont() << " denom: " << denom << endl;
  if (debug_level >= 550) cout << _unsegmented << "[" << i << "] : propto p(yes) = " << yes << ", p(no) = " << no << endl;
#endif
  //normalize
  yes = yes / (yes+no); 
  no = 1.0-yes;
#ifndef NDEBUG
  if (debug_level >= 500) cout << _unsegmented << "[" << i << "] : norm'zd p(yes) = " << yes << ", p(no) = " << no << endl;
#endif
  //do annealing
  yes = pow(yes, temp);
  no = pow(no, temp);
  //cout << "(" << yes << "," << no << ") ";
  Float p_yes = yes / (yes+no);
  //cout << p_yes << " ";
  if (randd() < p_yes) {
    _boundaries[i].yes = true;
    lexicon.inc(left);
    lexicon.inc(right);
  }
  else {
    _boundaries[i].yes = false;
    lexicon.inc(center);
  }
  //cout << endl;
}

//samples a single boundary point at position j
//with temperature temp (bigram model)
void
Utterance::sample_bigram(Count j, State& state, Float temp) {
  debug_output(800, "Utterance::sample_bigram:\n", *this);
  Lexicon& lexicon = state.get_lexicon();   
  BiLexicon& bilex = state.get_bilexicon();
  //indices are l,i,j,k,n in order from l to r.
  string li,kn;
  int i = prev_boundary(j);
  if (i < 0) {
    li = U_EDGE;
  }
  else {
    li = word_between(prev_boundary(i),i);
  }
  string ij = word_between(i,j);
  Count k = next_boundary(j);
  string jk = word_between(j,k);
  string ik = word_between(i,k);
  Count n_table;
  int n = -1;
  if (k == _boundaries.size()-1) {
    kn = U_EDGE;
    n_table = _boundaries[k].final_table;
  }
  else{
    n = next_boundary(k);
    kn = word_between(k,n);
    n_table =_boundaries[n].table;
  }
  Bigram lij(li,ij);
  Bigram ijk(ij,jk);
  Bigram jkn(jk,kn);
  Bigram lik(li,ik);
  Bigram ikn(ik,kn);
  if (_boundaries[j].yes) {
    //we don't dec li: cancels with "no" case in first
    // factor (and if U_EDGE, is annoying b/c not in lex).
    // will need to change this if doing MH.
    subtract_counts(lexicon, bilex,j,k,n,ij,jk,lij,ijk,jkn);
  }
  else {
    // see above.
    subtract_counts(lexicon, bilex,j,k,n,ik,lik,ikn);
  }
  Float yes;
  Float no;
  yes = compute_predictive(lij, state) *
    compute_predictive(ijk, state) * 
    compute_predictive(jkn, state);
  no = compute_predictive(lik, state) * 
    compute_predictive(ikn, state);
#ifndef NDEBUG
  if (debug_level >= 550) cout << ij << " " << lexicon(ij) << ", " << jk << " " << lexicon(jk) << ", " << ik << " " << lexicon(ik) << " " << state.alpha1() << endl;
   if (debug_level >= 550) cout << _unsegmented << "[" << j << "] : propto p(yes) = " << yes << ", p(no) = " << no << endl;
#endif
  //normalize
  yes = yes / (yes+no); 
  no = 1.0-yes;
#ifndef NDEBUG
  if (debug_level >= 500)
    cout << _unsegmented << "[" << j << "] : norm'zd p(yes) = " << yes << ", p(no) = " << no << endl;
#endif
  //do annealing
  yes = pow(yes, temp);
  no = pow(no, temp);
  Float p_yes = yes / (yes+no);
  // now choose table assignments
  if (randd() < p_yes) {  //we will end up with a boundary
    add_boundary(lexicon, bilex,j,k,n,ij,jk,lij,ijk,jkn,temp);
  }
  else {
    add_no_boundary(lexicon, bilex,j,k,n,ik,lik,ikn,temp);
  }
}

// subtracts unigram and bigram counts when there is
// already a boundary at j
void
Utterance::subtract_counts(Lexicon& lexicon, BiLexicon& bilex,
			   Count j, Count k, int n, 
			   const string& ij, const string& jk,
			   const Bigram& lij, const Bigram& ijk, const Bigram& jkn) {
  debug_output(800, "Utterance::subtract_count(yes): j=", j);
  _boundaries[j].yes = false;
  lexicon.dec(ij);
  lexicon.dec(jk);
  bilex.remove(lij, _boundaries[j].table);
  _boundaries[j].table = 0;
  bilex.remove(ijk, _boundaries[k].table);
  _boundaries[k].table = 0;
  if (n < 0) {
    bilex.remove(jkn, _boundaries[k].final_table);
    _boundaries[k].final_table = 0;
  }
  else {
    bilex.remove(jkn, _boundaries[n].table);
    _boundaries[n].table = 0;
  }
}

// subtracts unigram and bigram counts when there is
// no boundary at j
void
Utterance::subtract_counts(Lexicon& lexicon, BiLexicon& bilex,
			Count j, Count k, int n, 
			const string& ik,
			const Bigram& lik, const Bigram& ikn) {
  debug_output(800, "Utterance::subtract_counts(no): j=", j);
  lexicon.dec(ik);
  bilex.remove(lik, _boundaries[k].table);
  _boundaries[k].table = 0;
  if (n < 0) {
    bilex.remove(ikn, _boundaries[k].final_table);
    _boundaries[k].final_table = 0;
  }
  else {
    bilex.remove(ikn, _boundaries[n].table);
    _boundaries[n].table = 0;
  }
}


// replaces counts for bigrams assuming no
// boundary at position j, and samples tables for them.
// Assumes independence between these bigrams.
void
Utterance::add_no_boundary(Lexicon& lexicon, BiLexicon& bilex,
			   Count j, Count k, int n, 
			   const string& ik,
			   const Bigram& lik, const Bigram& ikn, Float temp) {
  debug_output(800, "Utterance::remove_boundary(): j=", j);
  _boundaries[j].yes = false;
  lexicon.inc(ik);
  _boundaries[j].table = 0;
  _boundaries[k].table = bilex.inc(lik, temp);
  if (n < 0) {
    _boundaries[k].final_table = bilex.inc(ikn, temp);
  }
  else {
    _boundaries[n].table = bilex.inc(ikn, temp);
  }
}

// adds a boundary at position j and
// samples tables for the new bigrams created.
// Assumes independence between these bigrams.
void
Utterance::add_boundary(Lexicon& lexicon, BiLexicon& bilex,
			Count j, Count k, int n, 
			const string& ij, const string& jk,
			const Bigram& lij, const Bigram& ijk, const Bigram& jkn, 
			Float temp) {
  debug_output(800, "Utterance::add_boundary(): j=", j);
  _boundaries[j].yes = true;
  lexicon.inc(ij);
  lexicon.inc(jk);
  _boundaries[j].table = bilex.inc(lij, temp);
  _boundaries[k].table = bilex.inc(ijk, temp);
  if (n < 0) {
    _boundaries[k].final_table = bilex.inc(jkn, temp);
  }
  else {
    _boundaries[n].table = bilex.inc(jkn, temp);
  }
}

//resample tables for boundary=yes case.
void
Utterance::sample_tables(BiLexicon& bilex, Count j, Count k, int n, 
			 const Bigram& lij, const Bigram& ijk, 
			 const Bigram& jkn, Float temp) {
  debug_output(800, "Utterance::sample_tables(yes): j=", j);
  bilex.remove(lij, _boundaries[j].table);
  _boundaries[j].table = bilex.inc(lij, temp);
  bilex.remove(ijk, _boundaries[k].table);
  _boundaries[k].table = bilex.inc(ijk, temp);
  if (n < 0) {
    bilex.remove(jkn, _boundaries[k].final_table);
    _boundaries[k].final_table = bilex.inc(jkn, temp);
  }
  else {
    bilex.remove(jkn, _boundaries[n].table);
    _boundaries[n].table = bilex.inc(jkn, temp);
  }
}

// resample tables for boundary = no case.
void
Utterance::sample_tables(BiLexicon& bilex, Count k, int n, 
			 const Bigram& lik, const Bigram& ikn, Float temp) {
  debug_output(800, "Utterance::sample_tables(no)", "");
  bilex.remove(lik, _boundaries[k].table);
  _boundaries[k].table = bilex.inc(lik, temp);
  if (n < 0) {
    bilex.remove(ikn, _boundaries[k].final_table);
    _boundaries[k].final_table = bilex.inc(ikn, temp);
  }
  else {
    bilex.remove(ikn, _boundaries[n].table);
    _boundaries[n].table = bilex.inc(ikn, temp);
  }
}

void
Utterance::sample_table(const BiLexicon& bilex, Count index,
			const Bigram& bg, Float temp, bool final) {
  /*
  // implementing bilex.sample_table is nontrivial
  Float old_table;
  if (final) {
    old_table = _boundaries[index].final_table;
  }
  else {
    old_table = _boundaries[index].table;
  }
  Float new_table = bilex.sample_table(bg, old_table, temp);
  if (old_table != new_table) {
    bilex.remove(bg, old_table);
    bilex.place(bg, new_table);
    if (final) {
      _boundaries[index].final_table = new_table;
    } 
    else {
      _boundaries[j].table = new_table;
    }
  }
  */
}

//basic numerator for sampling: 
//Count(wd) + alpha*p(wd)
Float
Utterance::numer_base(const string& wd, State& state) {
  /* //for unique lexicon
  Count n = (state.get_lexicon())(wd);
  if (n>0)
    return n;
  return state.p_word(wd);
  */
  //below is for non-unique lexicon
  typedef pair<string, CF> SCF;
  debug_output(550, "numer_base(): (wd, (count, p0(wd))) = ", SCF(wd, CF((state.get_lexicon())(wd), state.p_word(wd))));
  return (state.get_lexicon())(wd) + state.p_word(wd);
}

//predictive distribution for words in bigram model.
Float
Utterance::compute_predictive(const Bigram& bg, State& state, int table, Count denom_sub) const {
  Count prev_count;
  if (bg.first == U_EDGE) {
    prev_count = state.nutterances();
  }
  else {
    prev_count = (state.get_lexicon()) (bg.first);
  }
  return predictive_dist(bg, prev_count, state.get_bilexicon(),
			   table, denom_sub);
}

//predictive distribution for words in bigram model:
//(Count(w1,w2) + alpha1*p1(w2)) / Count(w1) + alpha1
//If table number is -1, as above.  If > -1, we
//subtract the item at that table from counts.
//Sometimes we also need to subtract 1 in denom, in
//which case denom_sub == 1.
Float
Utterance::predictive_dist(const Bigram& bg, Count prev_count,
				   const BiLexicon& bilex,
				   int table, Count denom_sub) const {
  typedef pair<Bigram, CF> BiCF;
  Count bg_count = bilex(bg);
  Float p;
  if (table < 0) {
    my_assert(denom_sub == 0, bg);
    p = (bg_count + State::p_word(bg, bilex)) /
      (prev_count + State::alpha1());
    debug_output(550, "predictive(): (bigram, (count, prob)) = ", BiCF(bg, CF(bg_count, p)));
    //    debug_output(550, "predictive(): (bigram, prob) = ", BiF(bg, p));
    debug_output(800, "predictive(): (Count(bg), Count(w1)) = ", CC(bg_count, prev_count));
  }
  else {
    p = (bg_count -1 + State::p_word(bg, bilex, table)) /
      (prev_count - denom_sub + State::alpha1());
    debug_output(550, "predictive(): (bigram, prob-) = ", CF(bg_count, p));
    //    debug_output(550, "predictive(): (bigram, prob-) = ", BiF(bg, p));
    debug_output(800, "predictive(): (Count(bg)-, Count(w1)-) = ", CC(bg_count -1, prev_count -denom_sub));
  }
  return p;
}

// predictive distribution for (word, table) in bigram model:
//(Count(w1,w2 at table)  / Count(w1) + alpha1    if Count > 0
// alpha1*p1(w2)) / Count(w1) + alpha1            else
Float
Utterance::joint_predictive_dist(const Bigram& bg, Count prev_count,
				 const BiLexicon& bilex, int table) const {
  typedef pair<Bigram, Count> BiC;
  typedef pair<BiC, Float> BiCF;
  Count table_count = bilex.ntokens(bg, table);
  Float numer;
  if (table_count == 0) {
    numer = State::p_word(bg, bilex);
  }
  else {
    numer = table_count;
  }
  Float p = numer / (prev_count + State::alpha1());
  debug_output(550, "joint_predictive(): ((bg, table), prob) = ", BiCF(BiC(bg, table), p));
  debug_output(800, "joint_predictive(): (Count(bg at table), Count(all)) = ", CC(table_count, prev_count));
  return p;
}

//returns the location of the boundary to left of pos. i (or -1 if none)
int
Utterance::prev_boundary(Count i) const {
  my_assert((i>=0) && (i<_unsegmented.size()), i);
  for (int j=i-1; j>=0; j--) {
    if (_boundaries[j].yes) return j;
  }
  return -1;
}

//returns the location of the boundary to right of pos. i
//i must be between 0 and _boundaries.size()-2 inclusive
//(i.e. don't call on final boundary at index _boundaries.size()-1.)
Count
Utterance::next_boundary(Count i) const {
  my_assert((i>=0) && (i<_unsegmented.size()-1), i);
  for (Count j=i+1; j<_boundaries.size(); j++) {
    if (_boundaries[j].yes) return j;
  }
  assert(0); //should  have found one by now
  return 0;
}
