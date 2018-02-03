#ifndef _SCORING_H_
#define _SCORING_H_

#include <iostream>
#include "Utterance.h"
#include "typedefs.h"

/*
Scoring class calculates number of words correct
in each utterance and keeps a running tally.
Calculates precision and recall, and lexicon precision.

*/
class Scoring {
public:
  Scoring():
    _utterances(0), _words_correct(0),
    _segmented_words(0), _reference_words(0),
    _bs_correct(0), _segmented_bs(0), _reference_bs(0) {}
  // find correct, segmented, and reference words
  // for utterance and add to totals. Add words
  // to seg lexicon.
  void score_utterance(Utterance* utterance);
  double precision() const {
    return (double)_words_correct/_segmented_words;}
  double recall() const {
    return (double)_words_correct/_reference_words;}
  double fmeas() const {
    return 2*recall()*precision()/(recall()+precision());}
  double b_precision() const {
    return (double)_bs_correct/_segmented_bs;}
  double b_recall() const {
    return (double)_bs_correct/_reference_bs;}
  double b_fmeas() const {
    return 2*b_recall()*b_precision()/(b_recall()+b_precision());}
  double lexicon_precision() const {
    return (double)lexicon_correct()/_segmented_lex.ntypes();}
  double lexicon_recall() const {
    return (double)lexicon_correct()/_reference_lex.ntypes();}
  double lexicon_fmeas() const{
    return 2*lexicon_precision()*lexicon_recall()/
      (lexicon_precision() + lexicon_recall());}
  void reset();
  void print_results(ostream& os=cout) const;
  void print_final_results(ostream& os=cout) const;
  void print_segmented_lexicon(ostream& os=cout) const;
  void print_reference_lexicon(ostream& os=cout) const;
  void print_segmentation_summary(ostream& os=cout) const {
    print_lexicon_summary(_segmented_lex);}
private:
  typedef StringLexicon Lexicon;
  int lexicon_correct() const;
  void add_words_to_lexicon(const Utterance::Words& words, Lexicon& lex);
  void print_lexicon(const Lexicon& lexicon,ostream& os=cout) const;
  void print_lexicon_summary(const Lexicon& lexicon,ostream& os=cout) const;
  int _utterances; // number of utterances so far in block
  int _words_correct;  // tokens
  int _segmented_words;
  int _reference_words;
  int _bs_correct;
  int _segmented_bs;
  int _reference_bs;
  Lexicon _segmented_lex;
  Lexicon _reference_lex;
};

#endif
