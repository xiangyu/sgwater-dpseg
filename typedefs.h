#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

#include <string>
#include <vector>
//#include <ext/hash_map>
#include "utils.h"
#include "math.h"
#include "SGLexicon.h"
#include "NGrams.h"
#include <unordered_map>

using namespace std;

typedef double Float;
typedef size_t Count;
typedef pair<string,Float> SF;
typedef pair<string,Count> SC;
typedef pair<Count, Count> CC;
typedef pair<Count, Float> CF;
typedef pair<Float, Float> FF;
typedef pair<bool, bool> BB;
typedef vector<SC> SCs;
typedef vector<CF> CFs;
typedef vector<Float> Fs;
typedef vector<Count> Cs;
typedef vector<bool> Bs;

// matrix of Floats. 
struct FFm : public std::vector<Fs> {
  FFm(Count n1, Count n2) : std::vector<Fs>(n1) {
    assert(size() == n1);
    for (Count i = 0; i < n1; ++i)
      operator[](i).resize(n2);
  }
  void resize(Count n1, Count n2, Float val=0) {
    std::vector<Fs>::resize(n1);
    for (size_t i = 0; i < size(); ++i)
      operator[](i).resize(n2,val);
  }
  Count size2() const {return begin()->size();}
};  // FFm{}

typedef SGLexicon<string,Count> StringLexicon;
class Lexicon;
class BiLexicon;

struct Boundary {
  bool yes; // word boundary?
  Count table; // table number
  Count final_table; // if this is an utt boundary,
  // final_table will be the number of the table with the
  // (last word, $) bigram.  Otherwise, = 0.
  Boundary(bool w, Count t = 0, Count f = 0):
    yes(w), table(t), final_table(f) {}
};

#endif
