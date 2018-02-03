#include "State.h"
#include "Urn.h"

extern Count debug_level;

// declare static variables
int State::_unigram_model = -1;
int State::_bigram_model = -1;
int State::_ngram = -1;
Float State::_noise = -1;
 Float State::_alpha = -1;
 Float State::_alpha1 = -1;
 Float State::_p_boundary = -1;
 Float State::_p_utt_boundary = -1;
 PhoneProbs State::_phoneme_ps;
 WordProbs State::_true_word_ps;
 BigramProbs State::_true_bg_ps;
StringLexicon State::_true_nfollow;

typedef pair<Bigram, Float> BiF;

void
State::set_models(string uni_model, string bi_model, int ngram, Float noise) {
  _ngram = ngram;
  _noise = noise;
  if (uni_model == "m")
    _unigram_model = MONKEYS;
  else if (uni_model == "m2")
    _unigram_model = VARI_MONKEYS;
  else 
    error("unknown unigram generator model\n");

  if (_ngram == 1) {
    // ignore bi_model.
    _bigram_model = MONKEYS;
  }
  else if (_ngram == 2) {
    //bigram model takes precedence over uni, if conflict
    if (bi_model == "m") {
      _bigram_model = MONKEYS;
      _unigram_model = MONKEYS;
    }
    else if (bi_model == "m2") {
      _bigram_model = VARI_MONKEYS;
      _unigram_model = VARI_MONKEYS;
    }
    else if (bi_model == "u") 
      _bigram_model = U_SAMPLE;
    else if (bi_model == "t") 
      _bigram_model = U_TABLES;
     else if (bi_model == "y")
      _bigram_model = U_TYPES;
    else if (bi_model == "k")
      _bigram_model = U_TOKENS;
    else if (bi_model == "b")
      _bigram_model = B_TYPES;
    else
      error("unknown bigram generator model\n");
  } 
  else {
    error("ngram must be 1 or 2\n");
  }
}

//alpha is the Dirichlet hyperparam, b is the prior prob. of a boundary.
//alpha1 is the bigram Dirichlet, p_utt_b is prior prob of utt boundary.
State::State(DatafileBase* data, Float alpha, Float b, Float alpha1, Float p_utt_b):
  _nutterances(0),
  _bg_counts(this) {
  _alpha = alpha;
  _alpha1 = alpha1;
  _p_boundary = b;
  _p_utt_boundary = p_utt_b;
  my_assert((_p_boundary > 0) && (_p_boundary <=1), _p_boundary);
  //  _bg_counts.set_min_table_count(_alpha1);
  assert((_unigram_model >= MONKEYS) && 
	 (_bigram_model >= MONKEYS) && 
	 (_bigram_model <= B_TYPES));
  switch (_unigram_model) {
  case MONKEYS: cout << "Unigram generator: monkeys" << endl; break;
  case VARI_MONKEYS: cout << "Unigram generator: monkeys" << endl; break;
  }
  cout << " alpha0: " << _alpha << ", ";
  cout << "p_boundary: " << _p_boundary << endl;
  if (_ngram == 2) {
    if (_bigram_model != U_TABLES) {
      //haven't updated remaining models, so print error.
      cerr << "Bigram generator must be set to unigram tables (-ut)" << endl;
      cout << "Bigram generator must be set to unigram tables (-ut)" << endl;
      exit(0);
    }
    switch (_bigram_model) {
    case MONKEYS: cout << "Bigram generator: monkeys" << endl; break;
    case VARI_MONKEYS: cout << "Bigram generator: monkeys" << endl; break;
    case U_SAMPLE : cout << "Bigram generator: unigram sampling (tokens)" << endl; break;
    case U_TABLES : cout << "Bigram generator: unigram sampling (tables)" << endl; break;
    case U_TYPES : 
      cout << "Bigram generator: unigram types" << endl;
      cout << " Noise parameter: " << _noise << ", "; break;
    case U_TOKENS : 
      cout << "Bigram generator: unigram tokens" << endl; 
      cout << " Noise parameter: " << _noise << ", "; break;
    case B_TYPES : 
      cout << "Bigram generator: bigram types" << endl;
      cout << " Noise parameter: " << _noise << ", "; break;
    }
    cout << " alpha1: " << _alpha1;
    cout << ", P($): " << _p_utt_boundary << endl;
  }
  cout << "Sampling of hyperparameters: ";
  if (SAMPLE_HYPERPARAMETERS)
    cout << "ON" << endl;
  else
    cout << "OFF" << endl;
  unordered_map<char,Count> alphabet;
  string s;
  s = data->next_reference();
  while (!s.empty()) {
    Utterance u(s,b); //generates utterance w/ random segm.
     _utterances.push_back(u);
    _nutterances++;
    string unsegmented = u.get_unsegmented();
    for (Count i=0; i<unsegmented.length(); i++) {
      alphabet[unsegmented[i]] = 1;
    }
    s = data->next_reference();
  }
  _alphabet_size = alphabet.size();
  init_probs(); //need to do this before adding counts
  // because it initializes phoneme probabilities, which
  // are needed for backoff probs when choosing tables.
  foreach (Utterances, u, _utterances) {
    u->add_counts_to_lex(_word_counts, _bg_counts, _ngram);
  }
  //  cout << _smooth << endl;
}

void
State::init_probs() {
  cforeach(Utterances, u, _utterances) {
    //add true words to lex
    Utterance::Words wds = u->get_reference_words();
    string prev = U_EDGE;
    cforeach(Utterance::Words, w, wds) {
      _true_word_ps.inc(*w);
      bool inserted =_true_bg_ps.inc(Bigram(prev,*w));
      if (inserted) _true_nfollow.inc(prev);
      prev = *w;
    }
    bool inserted =_true_bg_ps.inc(Bigram(prev,U_EDGE));
    if (inserted) _true_nfollow.inc(prev);
  }
  //normalize - give empirical prob over unigrams
  foreach(WordProbs, w, _true_word_ps) {
    Float ntokens = _true_word_ps.ntokens();
    if (_bigram_model == U_TYPES)
      w->second = 1.0/_true_word_ps.ntypes();
    else 
      w->second /= ntokens;
  }
  //normalize - give uniform prob over bigram types
  foreach(BigramProbs, b, _true_bg_ps) {
    b->second = 1.0/_true_nfollow(b->first.first);
  }
  //   cout << _bg_counts << endl;
  //   cout << _true_word_ps << endl;
  //  cout << _true_bg_ps << endl;
  //  cout << _true_nfollow << endl;
  init_phoneme_probs();
}

// prior prob of word w with length n =
// alpha * p(n) * \prod_1^n p(w_i)
// In bigram model, may need to compute p_word for utt boundaries,
// and we also need to discount reg. words by the mass of utt b's.
Float 
State::p_word(const string& s) {
  Float p=1;
  if (s == U_EDGE) {
    p = _alpha*prior_utt_boundary();
  }
  else {
    PhoneProbs::const_iterator i;
    cforeach (string, c, s) {
      i = _phoneme_ps.find(*c);
      // make sure we have init'd all phoneme probs in this word.
      // (watch out for words not in the training data file)
      my_assert(i != _phoneme_ps.end(), s);
      p *= i->second;
    }
    p *= _alpha* pow(1 - _p_boundary, (int)s.size()-1) * _p_boundary;
    if (_ngram == 2)
      p *= 1-prior_utt_boundary();
  }
  debug_output(1000, "uni p_word(): (w, p(w)) = ", SF(s,p));
  return p;
}

//generator prob of word w_j given prev word w_i
//If table == - 1, we compute the probability for
//adding a new bg.
//If table != -1, bg is already assigned to table,
//so we subtract that table when computing.
Float
State::p_word(const Bigram& bg, const BiLexicon& bg_lexicon, int table) {
  my_assert(!(bg.first == U_EDGE && bg.second == U_EDGE), bg);
  my_assert(_bigram_model == U_TABLES, "update p_word() or use -ut!\n");
  Count ntables = bg_lexicon.ntables();
  Count ntables_word = bg_lexicon.ntables(bg.second);
  if (table >= 0 && 
      bg_lexicon.ntokens(bg, (Count)table) == 1) {
    ntables -= 1;
    ntables_word -= 1;
  }
  debug_output(1000, "bi p_word(): (t(all), t(w2)) = ", 
	       CC(ntables, ntables_word));
  Float p = alpha1()* (ntables_word + p_word(bg.second)) /
    (ntables + _alpha);
  debug_output(800, "bi p_word() ((w1, w2), p1(w1,w2)) = : ", BiF(bg, p));
  return p;
}

// use annealing temperature
void
State::sample(Float temp) {
  _word_counts.check_invariant();
  foreach(Utterances, u, _utterances) {
    u->sample(*this, temp, _ngram);
  }
  if (SAMPLE_HYPERPARAMETERS)
    hypersample(temp);
}

//sample hyperparameters: 
//alpha,  alpha1, p_boundary, p_utt_boundary
void 
State::hypersample(Float temp){
  bool changed = sample_hyperparm(_alpha, false, temp);
  //  changed ? cout << "new alpha0: " << _alpha << endl : cout << "old alpha0: " << _alpha << endl;
  if (_ngram == 2) {
    changed = sample_hyperparm(_alpha1, false, temp);
    //    changed ? cout << "new alpha1: " << _alpha1 << endl : cout << "old alpha1: " << _alpha1 << endl;
  }
  changed = sample_hyperparm(_p_boundary, true, temp);
  //  changed ? cout << "new p_boundary: " << _p_boundary << endl : cout << "old p_boundary: " << _p_boundary << endl;
  changed = sample_hyperparm(_p_utt_boundary, true, temp);
  //  changed ? cout << "new p_utt_boundary: " << _p_utt_boundary << endl : cout << "old p_utt_boundary: " << _p_utt_boundary << endl;
}

//beta is the hyperparameter to be sampled.
//assume beta must be > 0.  If beta must be < 1, set flag.
// returns true if value of beta changed.
bool
State::sample_hyperparm(Float& beta, bool is_prob, Float temp) {
  Float std_ratio = HYPERSAMPLING_RATIO;
  Float old_beta = beta;
  Float new_beta;
  if (is_prob && old_beta > .5) {
    new_beta = rand_normal(old_beta, std_ratio*(1-old_beta));
  }
  else {
    new_beta = rand_normal(old_beta, std_ratio*old_beta);
  }
  if (new_beta <= 0 || (is_prob && new_beta >= 1)) {
    error("beta out of range\n");
    return false;
  }
  Float old_p = log_posterior();
  beta = new_beta;
  Float new_p = log_posterior();
  Float r = exp(new_p-old_p)*
    normal_density(old_beta, new_beta, std_ratio*new_beta)/
    normal_density(new_beta, old_beta, std_ratio*old_beta);
  r = pow(r,temp);
  //cout << old_beta << ", " << new_beta << ", " << old_p << ", " << new_p << ", " << new_p-old_p << ", " << r << ", ";
  //if (new_beta < old_beta) cout << "-, "; else cout << "+, ";
  bool changed = false;
  if ((r >= 1) || (r >= randd(1))) {
    //cout << "+";
    changed = true;
  }
  else {
    //cout << "-";
    beta = old_beta;
  }
  //cout << ";  %beta(o/n), P(o/n), diff, r, up/do, ac/re" << endl;
  return changed;
}


// generates (prints) a single random utterance
void
State::generate() const {
  if (_ngram == 1) {
    cout << generate_word();
    Float p_stop = 1.0 - 
      p_cont2(_word_counts.ntokens(), _nutterances);
    while (randd() >= p_stop) {
      cout << " " << generate_word();
    }
    cout << endl;
  }
  else if (_ngram == 2) {
    string previous = generate_word(U_EDGE);
    cout << previous;
    while (previous != U_EDGE) {
      string current = generate_word(previous);
      if (current != U_EDGE) {
	cout << " " << current;
      }
      previous = current;
    }
    cout << endl;
  }
}

//unigram
string
State::generate_word() const {
    Urn<string, Float> words;
    words.reserve(_word_counts.ntypes()+1);
    cforeach(Lexicon, j, _word_counts) {
      words.push(j->first, j->second);
    }
    words.push(generate_novel_word(), alpha());
    return words.draw();
}

//bigram
string
State::generate_word(const string& previous) const {
  // previous word was novel
  if (_word_counts(previous) == 0 && previous != U_EDGE) {
    return generate_novel_second(previous);
  }
  else {
    Urn<string, Float> words;
    words.reserve(_bg_counts.ntypes()+ 1);
    cforeach(BiLexicon, j, _bg_counts) {
      if (j->first.first == previous) {
	words.push(j->first.second, j->second);
      }
    }
    words.push(generate_novel_second(previous), alpha1());
    return words.draw();
  }
}

// generates the second word of a novel bigram
string
State::generate_novel_second(const string& previous) const {
  if (_bigram_model == U_TABLES) {
    Urn<string, Float> words;
    words.reserve(_word_counts.ntypes());
    cforeach(Lexicon, j, _word_counts) {
      words.push(j->first,
		 _bg_counts.ntables(j->first));
    }
    words.push(generate_novel_word(), alpha());
    if (previous != U_EDGE) {
      words.push(U_EDGE,
		 _bg_counts.ntables(U_EDGE));
    }
    return words.draw();
  }
  else {
    cerr << "generate_word is only implemented for -u" << endl;
    exit(0);
  }
}

Float
State::log_posterior() const {
  Float prob = 0;
  Count nutts = 0;
  //start w/ empty lexicon
  Lexicon lexicon;
  BiLexicon bilexicon;
  //  bilexicon.set_min_table_count(_alpha1);
  cforeach(Utterances, u, _utterances) {
    if (_ngram == 1)     //unigram
      prob += u->log_posterior(nutts, lexicon, *this);
    else if (_ngram == 2)    //for bigram model
      prob += u->log_posterior(nutts, lexicon, bilexicon, *this);
    else
      my_assert(0,"unknown model in State::log_posterior");
    nutts++;
  }
  return prob;
}

void
State::init_phoneme_probs() {
    //determine phoneme counts in true word types
  StringLexicon alphabet;
    cforeach(WordProbs, w, _true_word_ps) {
      for (Count i=0; i<w->first.length(); i++) {
	alphabet.inc(w->first.substr(i,1));
      }
    }
  //init w/ true distr. -- doesn't make much diff.
  if ((_unigram_model == VARI_MONKEYS) ||
      (_bigram_model == VARI_MONKEYS)) {
    cout << "Phoneme distribution: true" << endl;
    cforeach(StringLexicon, c, alphabet) {
      _phoneme_ps[c->first[0]] = (double)alphabet(c->first)/alphabet.ntokens();
    }
  }
  else {  //init w/ uniform distr.
    cout << "Phoneme distribution: uniform" << endl;
    cforeach(StringLexicon, c, alphabet) {
      _phoneme_ps[c->first[0]] = 1.0/_alphabet_size;
    }
  }
  assert(_phoneme_ps.size() == _alphabet_size);
}

void
State::print_stats (ostream& os) const {
  Count op = os.precision();
  os.precision(6);
  os.width(os.precision());
  os << left << p_cont() << ", ";
  os.width(1);
  os << _word_counts.ntypes() << ", " << _word_counts.ntokens() << ", "
     << _bg_counts.ntypes() << ", " << _bg_counts.ntokens() << ", "
     << _bg_counts.ntables() << ", ";
  os.width(os.precision());
  os << left << -1*log_posterior() << ", "
     << _alpha << ", " << _alpha1 << ", " 
     << _p_boundary << ", " << _p_utt_boundary << ";" << endl;
  os.width(1);
  os.precision(op);
}

void
State::print_stats_header (ostream& os) const {
  os << "% iter, p_cont, types, tokens, "
     << "bitypes, bitokens, bitables, neglogP, "
     << "a0, a1, p_boundary, p_utt_boundary" << endl;
}
