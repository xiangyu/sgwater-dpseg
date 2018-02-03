#include <string>
#include "Scoring.h"

using namespace std;

void
Scoring::score_utterance(Utterance* utterance)
{
  _utterances++;
  add_words_to_lexicon(utterance->get_segmented_words(),
		       _segmented_lex);
  add_words_to_lexicon(utterance->get_reference_words(),
		       _reference_lex);
  // calculate number of correct words, segmented words,
  // and reference words and add to totals
  string segmented = utterance->get_segmented();
  string reference = utterance->get_reference();
  string::const_iterator s = segmented.begin();
  string::const_iterator r = reference.begin();
  bool left_match = 1;
  while (s != segmented.end()) {
    if ((*s == SENTINEL) && (*r == SENTINEL)) {
      _bs_correct++;
      _segmented_bs++;
      _reference_bs++;
      if (left_match) {
	_words_correct++;
      }
      left_match = 1;
      _segmented_words++;
      _reference_words++;
      s++;
      r++;
    }
    else if (*s == SENTINEL) {
      _segmented_words++;
      _segmented_bs++;
      left_match = 0;
      s++;
    }
    else if (*r == SENTINEL) {
      _reference_words++;
      _reference_bs++;
      left_match = 0;
      r++;
    }
    else {
      s++;
      r++;
    }
  }
  //subtract right utt boundary
  _bs_correct--; 
  _segmented_bs--;
  _reference_bs--;
}

void
Scoring::reset()
{
  _utterances = 0;
  _words_correct = 0;
  _segmented_words = 0;
  _reference_words = 0;
  _bs_correct = 0;
  _segmented_bs = 0;
  _reference_bs = 0;
  _reference_lex.clear();
  _segmented_lex.clear();
}

void
Scoring::add_words_to_lexicon(const Utterance::Words& words, Lexicon& lex)
{
  cforeach(Utterance::Words, iter, words) {
    lex.inc(*iter);
  }
}

void
Scoring::print_results(ostream& os) const
{
  os.precision(4);
  os << "P " << 100*precision()
       << " R " << 100*recall()
       << " F " << 100*fmeas()
       << " BP " << 100*b_precision()
       << " BR " << 100*b_recall()
       << " BF " << 100*b_fmeas()
       << " LP " << 100*lexicon_precision()
       << " LR " << 100*lexicon_recall()
       << " LF " << 100*lexicon_fmeas()
       << endl;
  os.precision(6);
}

void
Scoring::print_final_results(ostream& os) const
{
  os << "Reference lexicon summary:" << endl;
  print_lexicon_summary(_reference_lex);
  os << endl;
  os << "Segmented lexicon summary:" << endl;
  print_lexicon_summary(_segmented_lex);
  os << endl;
  print_results();
}

/*
Looks up each word in the segmented lexicon to see
whether it's also in the reference lexicon.
We need to calculate this each time it's called because
the number of correct words and lexicon size changes.
*/
int
Scoring::lexicon_correct() const
{
  int correct = 0;
  cforeach(Lexicon, iter, _segmented_lex) {
    if (_reference_lex.count(iter->first))
      correct++;
  }
  return correct;
}

void
Scoring::print_segmented_lexicon(ostream& os) const
{
  os << "Segmented lexicon:" << endl;
  print_lexicon(_segmented_lex);
  os << endl;
  os << "Total segmented lexicon types: " << _segmented_lex.ntypes() << endl;
  os << "Total segmented lexicon tokens: " << _segmented_words << endl;
}

void
Scoring::print_reference_lexicon(ostream& os) const
{
  os << "Reference lexicon:" << endl;
  print_lexicon(_reference_lex);
  os << endl;
  os << "Total reference lexicon types: " << _reference_lex.ntypes() << endl;
  os << "Total reference lexicon tokens: " << _reference_words << endl;
}

void
Scoring::print_lexicon(const Lexicon& lexicon, ostream& os) const
{
  //  Cs length_counts(Utterance::MAX_LENGTH, 0);
  vector<SC> word_counts;
  Count tot = 0;
  cforeach(Lexicon, iter, lexicon) {
    word_counts.push_back(SC(*iter));
    tot += iter->second;
  }
  sort(word_counts.begin(), word_counts.end(),
       second_greaterthan());
  foreach(vector<SC>, iter, word_counts) {
    if (_reference_lex.count(iter->first))
      os << "+ ";
    else
      os << "- ";
    os << iter->second << " " << iter->first << endl;
  }
  print_lexicon_summary(lexicon);
}

void
Scoring::print_lexicon_summary(const Lexicon& lexicon, ostream& os) const
{
  Cs length_counts(Utterance::MAX_LENGTH, 0);
  Cs length_types(Utterance::MAX_LENGTH, 0);
  Count tot = 0;
  cforeach(Lexicon, iter, lexicon) {
    length_counts[iter->first.length()] += iter->second;
    tot += iter->second;
    length_types[iter->first.length()]++;
  }
  os << endl << "Total words of each length (token/type): " << endl;
  Float total_counts = 0;
  Float total_types = 0;
  for (Count i=0; i<length_counts.size(); i++) {
    if (length_counts[i] > 0) {
      total_counts += length_counts[i]*i;
      total_types += length_types[i]*i;
      os << i << ": " << length_counts[i] << " ("
	   << 100.0*length_counts[i]/tot << "%) / "
	   << length_types[i] << " (" 
	   << 100.0*length_types[i]/lexicon.ntypes() << "%)" << endl;
    }
  }
  os << "Average word length: " 
       << total_counts/tot << " / "
       << total_types/lexicon.ntypes() << endl;
}

