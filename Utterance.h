#ifndef _UTTERANCE_H
#define _UTTERANCE_H

//#include <iostream.h>
#include <string>
#include <vector>
#include <list>
#include "utils.h"
#include "typedefs.h"

/*
Utterance class represents a single utterance, initially
containing a _reference transcription (correct segmentation)
and _unsegmented transcription as strings.  _boundaries
represents the location of word boundaries in the segmented
string (initially random, then resampled), with value "true"
indicating a boundary AFTER the i'th character.  so there is
a "true" at the final position, but possibly not at i=0.
_reference uses SENTINEL character as a word separator.

get_reference_words() and get_segmented_words() return the same 
information as lists of strings.  _reference_words is stored, but
segmented words change often, so we don't store them.  (These are
now only used for ease in scoring).

_score is the score generated by the segmenting algorithm.
*/

using namespace std;
class State;
extern Count debug_level;

// indicates edge of word
// must be a character not contained in any word
const char SENTINEL = '|';
// indicates edge of utt
const string U_EDGE = "$$";

class Utterance;
typedef list<Utterance> Utterances;

class Utterance {
public:
  // maximum number of characters per utterance
  static const int MAX_LENGTH = 500;
  //  typedef std::vector<string> Words;
  class Words: public std::vector<string> {
  public:
    //second from front/back
    string front_2() const {return *(begin() + 1);}
    string back_2() const {return *(end() - 2);}
  };
  typedef Words::const_iterator Words_iterator;
  
  Utterance(const string& reference, Float p_segment=.2);
  //type of initialization for boundaries: "ran", "pho", or "utt".
  static void set_init(string b_init); 
  void add_counts_to_lex(Lexicon& word_counts, BiLexicon& bg_counts, Count model = 1);
  string get_reference() const {return _reference;}
  string get_unsegmented() const {return _unsegmented;}
  string get_segmented() const;
  double get_score() const {
    if (_score < 0)
      cerr << "Warning: Accessing uninitialized score\n";
    return _score;}
  const Words& get_reference_words() const {return _reference_words;}
  Words get_segmented_words();
  void set_score(double score) {_score = score;}
  //do Gibbs sampler with annealing temperature, and ngram model
  void sample(State& state, Float temp=1, Count model=1); 
  //for unigram model (nutts is the # utts before this one.)
  Float log_posterior (Count nutts, Lexicon& lex, const State& state) const;
  //for bigram model
  Float log_posterior (Count nutts, Lexicon& lex, BiLexicon& bilex, const State& state) const;
  void print_reference(ostream& os=cout) const {os << _reference << '\n';}
  void print_segmented(ostream& os=cout) const {os << get_segmented() << '\n';}
  void print_unsegmented(ostream& os=cout) const {os << _unsegmented << '\n';}
  friend ostream& operator<< (ostream& os, const Utterance& u) {
#ifdef NDEBUG
    return u.print_basic(os);
#else
    return u.print_debug(os);
#endif
  }
private:
  ostream& print_basic(ostream& os) const {
      string utt = get_segmented();
      for (Count i=0; i<utt.size()-1; i++) {
	if (utt[i] == SENTINEL)
	  os.put(' ');
	else
	  os.put(utt[i]);
      }
      return os;
  }
  ostream& print_debug(ostream& os) const {
    if (debug_level > 800) {
    os << _unsegmented << endl;
    for (Count i=0; i<_boundaries.size(); i++) os << _boundaries[i].yes;
    os << endl;
    for (Count i=0; i<_boundaries.size(); i++) os << _boundaries[i].table;
    os << endl;
    for (Count i=0; i<_boundaries.size(); i++) os << _boundaries[i].final_table;
    os << endl;
    return os;
    }
    return print_basic(os);
  }
  //sample one boundary at pos'n i w/ temperature temp
  void sample_one(Count i, State& state, Float temp = 1); 
  //sample one boundary in bigram model
  void sample_bigram(Count i, State& state, Float temp = 1); 
  void sample_table(const BiLexicon& bilex, Count index,
		    const Bigram& bg, Float temp = 1, bool final = false);
  void subtract_counts(Lexicon& lexicon, BiLexicon& bilex,
		    Count j, Count k, int n, 
		    const string& ik,
		    const Bigram& lik, const Bigram& jkn);
  void subtract_counts(Lexicon& lexicon, BiLexicon& bilex,
		    Count j, Count k, int n, 
		    const string& ij, const string& jk,
		    const Bigram& lij, const Bigram& ijk, 
		    const Bigram& jkn);
  void add_no_boundary(Lexicon& lexicon, BiLexicon& bilex,
		    Count j, Count k, int n, 
		    const string& ik,
		    const Bigram& lik, const Bigram& jkn, Float temp = 1);
  void add_boundary(Lexicon& lexicon, BiLexicon& bilex,
		    Count j, Count k, int n, 
		    const string& ij, const string& jk,
		    const Bigram& lij, const Bigram& ijk, 
		    const Bigram& jkn, Float temp = 1);
  void sample_tables(BiLexicon& bilex, Count j, Count k, int n, 
		     const Bigram& lij, const Bigram& ijk, 
		     const Bigram& jkn, Float temp);
  void sample_tables(BiLexicon& bilex, Count k, int n, 
		     const Bigram& lik, const Bigram& jkn, Float temp);
  inline Float numer_base(const string& wd, State& state);
  //When table >= 0, we subtract counts from that table when
  //computing predictive dist.
  Float compute_predictive(const Bigram& bg, State& state, 
		   int table = -1, Count denom_sub = 0) const;
  Float predictive_dist(const Bigram& bg, Count prev_count,
			  const BiLexicon& bilex,
			  int table = -1, Count denom_sub = 0) const;
  // used in log posterior computation
  Float joint_predictive_dist(const Bigram& bg, Count prev_count,
			      const BiLexicon& bilex, int table) const;
  //return the string from prev boundary to i.
  string left_word(Count i) const {
    my_assert((i>=0) && (i<_unsegmented.length()), i);
    int prev = prev_boundary(i);
    return _unsegmented.substr(prev+1, i-prev);}
  //return the string from i to next boundary.
  string right_word(Count i) const {
    my_assert((i>=0) && (i<_unsegmented.length()-1), i);
    Count next = next_boundary(i);
    return _unsegmented.substr(i+1, next-i);}
  //return the string around i from prev boundary to next boundary.
  string center_word(Count i) const {
    my_assert((i>=0) && (i<_unsegmented.length()-1), i);
    return word_between(prev_boundary(i), next_boundary(i));
  }
  // returns word between prev. and next boundaries
  string word_between(int prev, Count next) const {
    return _unsegmented.substr(prev+1, next-prev);
  }
  //returns -1 if prev. boundary is beg. of utt.
  int prev_boundary(Count i) const;
  Count next_boundary(Count i) const;
  string _reference;
  string _unsegmented;
  typedef vector<Boundary> Boundaries;
  Boundaries _boundaries; //is there a boundary after the i'th char?
  double _score;
  Words _reference_words;
  static int _init;
  static const int TRUE_INIT = 3;
  static const int UTT_INIT = 2;
  static const int PHO_INIT = 1;
  static const int RAN_INIT = 0;
};

#endif