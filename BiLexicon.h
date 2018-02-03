#ifndef _BILEXICON_H_
#define _BILEXICON_H_

#include <iostream>
#include "SGLexicon.h"
#include "typedefs.h"
#include "NGrams.h"
#include "Restaurant.h"

/*
The bigram lexicon keeps track of bigrams, and also
estimates the number of tables on which each word
sits.
*/

class BiLexicon: public SGLexiconBase<Bigram, Count> {
private:
  typedef SGLexiconBase<Bigram, Count> Parent;
  typedef pair<Bigram, Count> BiC;
public:
  BiLexicon(const State* state) {Restaurant::STATE = state;}
  BiLexicon() {}
  virtual ~BiLexicon() {}
  virtual void check_invariant() const;
  virtual void clear() {
    Parent::clear();
    _tables.clear();
  }
  // adds to a random table
  //returns table number
  virtual size_t inc(const Bigram& pair) {return inc(pair, 1);}
  virtual size_t dec(const Bigram& pair) {return dec(pair, 1);}
  virtual size_t inc(const Bigram& pair, Float temp);
  virtual size_t dec(const Bigram& pair, Float temp);
  // adds one token of s to specified table
  // returns true if a new table was created.
  // set unsafe->true only in log posterior when tables may be incr'd out of order.
  bool place(const Bigram& b, Count table, bool unsafe=false);
  // removes one token from specified table
  // returns true if table was deleted
  bool remove(const Bigram& b, Count table);
  virtual Count ntables(const string& s) const {
    return _tables(s);
  }
  virtual Count ntables() const {
    return _tables.ntokens();
  }
  virtual Count ntokens() const {
    return Parent::ntokens();
  }
  virtual Count ntokens(const Bigram& b, Count table) const {
    Restaurants::const_iterator i = _restaurants.find(b);
    if (i == _restaurants.end()) return 0;
    return i->second.ntokens(table);
  }
  void print(ostream& os=cout) {
    os << "Restaurants: " << endl;
    cforeach (Restaurants, r,_restaurants) {
      os << r->second << endl;
    }
    os << "Tables: " << endl <<  _tables << endl;
  }
private:
  // keep track of how many tables this
  // word is on, among all preceding words.
  typedef SGLexicon<string, Count> Tables;
  Tables _tables;
  // not quite a "restaurant" in the HDP sense,
  // since each restaurant tracks tables for only
  // a single bigram.
  typedef std::unordered_map<Bigram, Restaurant> Restaurants;
  typedef pair<Bigram, Restaurant> Restaurant_pair;
  Restaurants _restaurants;
};


#endif
