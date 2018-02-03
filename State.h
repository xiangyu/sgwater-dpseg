#ifndef _STATE_H_
#define _STATE_H_

#include <iostream>
#include "typedefs.h"
#include "Datafile.h"
#include "Scoring.h"
#include "BiLexicon.h"

/* State keeps track of global state of the current hypothesis
for Gibbs sampler, as well as values of hyperparameters.
Contains a list of all utterances being segmented, and a Lexicon
storing the number of words of each type in the current hypothesis.
*/

using namespace std;
extern Float HYPERSAMPLING_RATIO; // the standard deviation for new hyperparm proposals
extern bool SAMPLE_HYPERPARAMETERS;

typedef unordered_map<char,Float> PhoneProbs;
typedef SGLexicon<string,Float> WordProbs;
typedef SGLexicon<Bigram,Float> BigramProbs;

class Lexicon: public SGLexicon<string,Count> {
public:
  Lexicon() {}
  virtual ~Lexicon() {}
  virtual Count operator()(const string& s) const {
    my_assert(s != U_EDGE, "Do not search for $$ in Lexicon!\n");
    return SGLexicon<string, Count>::operator() (s);
  }
};

class State {
public:
  // sets the unigram and bigram generator models
  // and whether to use unigram or bigram sampling.
  // Call this before calling constructor.
  static void set_models(string uni, string bi, int ngram, Float noise=.0001);
  //data is to read utterances from,
  //alpha is the Dirichlet hyperparam,
  //b is the prior prob. of a boundary.
  State(DatafileBase* data, Float alpha, Float b, Float alpha1, Float p_utt_boundary);

    //total weight on words in unigram generator
  static Float alpha() {return _alpha;}
  //total weight on words in bigram generator
  static Float alpha1() {return _alpha1;}
  //total smoothing weight on S -> W S
  static Float beta() {return 2;}
  static Float prior_utt_boundary() {return _p_utt_boundary;}
  static Float p_boundary() {return _p_boundary;}
// probability of S -> W S for unigram sampler
  // (where one extra rule must be added).
  static Float p_cont(Count nwds, Count nutts) {
    Float p_cont = Float(nwds - nutts + 1 + beta()/2) /
      (nwds + 1 + beta());
    my_assert((p_cont < 1) &&(p_cont > 0), p_cont);
    return p_cont;
  }
  Float p_cont() const {
    return p_cont(_word_counts.ntokens(), _nutterances);
  }
  // probability of S -> W S for log posterior
  static Float p_cont2(Count nwds, Count nutts ) {
    Float p_cont = Float(nwds - nutts + beta()/2) /
      (nwds + beta());
    my_assert((p_cont < 1) &&(p_cont > 0), p_cont);
    return p_cont;
  }
  // table probability of $ in bigram model
  static Float p_stop_tables(const BiLexicon& bg_lexicon) {
    return (bg_lexicon.ntables(U_EDGE) + beta()/2) /
      (bg_lexicon.ntables() + beta());
  }
  //prior prob of a word (alpha * generator prob)
  static Float p_word(const string& s);
  //prior prob of a word given previous word.
  // table is current table of bg. (-1 if adding new bg)
  Float p_word(const Bigram& bg, int table = -1) const {
    return p_word(bg, _bg_counts, table);
  }
  static Float p_word(const Bigram& bg, 
		      const BiLexicon& bg_lexicon, int table = -1);
  Float alphabet_size() const {return _alphabet_size;}
  Lexicon& get_lexicon() {return _word_counts;}
  BiLexicon& get_bilexicon() {return _bg_counts;}
  const Count nutterances() {return _nutterances;}
  //use annealing temperature temp
  void sample(Float temp=1);
  void hypersample(Float temp);
  void generate() const;
  Float log_posterior() const;
  void score_utterances(Scoring& scoring) {
    foreach(Utterances, u, _utterances) {
      scoring.score_utterance(&(*u));
    }
  }
  void print_stats (ostream& os) const;
  void print_stats_header (ostream& os) const;
  friend ostream& operator<< (ostream& os, const State& state) {
    cforeach(Utterances, i, state._utterances) {
      os << (*i) << endl;
    }
    return os;
  }
private:
  Utterances _utterances;
  Count _nutterances;
  Count _alphabet_size;
  Lexicon _word_counts;
  BiLexicon _bg_counts;

  enum {MONKEYS, VARI_MONKEYS,
	U_SAMPLE, U_TABLES, U_TOKENS, U_TYPES, B_TYPES};
  static int _unigram_model;
  static int _bigram_model;
  static int _ngram; //which model to use (1 or 2)
  static Float _noise; //how much noise to use when generators use true forms.
  static Float _alpha; //total weight of unigram generator
  static Float _alpha1; //total weight of bigram generator
  static Float _p_boundary;
  static Float _p_utt_boundary;
  static PhoneProbs _phoneme_ps;
  static WordProbs _true_word_ps; //true words in data
  static BigramProbs _true_bg_ps; //true bigrams in the data
  static StringLexicon _true_nfollow; //number of types following each type in true data.
  void init_probs();
  void init_phoneme_probs();
  bool sample_hyperparm(Float& beta, bool is_prob, Float temp=1);
  string generate_word() const;
  string generate_word(const string& previous) const;
  string generate_novel_word() const {return "NOVEL";};
  string generate_novel_second(const string& previous) const;
};


#endif
