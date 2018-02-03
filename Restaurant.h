#ifndef _RESTAURANT_H_
#define _RESTAURANT_H_

#include <iostream>
#include "utils.h"
#include "typedefs.h"
#include "Urn.h"

/*
Restaurant represents a lexical item and keeps track of
the number of tables and how many tokens are on each.
Tables are implemented as a hash Count->Count where
the keys are table indices.  When a table is emptied,
its index is stored in a list for reuse.
*/

class State;
class BiLexicon;
//typedef ext::hash_map<Count, Count> Tables;
typedef unordered_map<Count, Count> Tables;
//typedef Urn<Sample, Float> Samples;

class Restaurant {
public:
  static const State* STATE;
  Restaurant(Bigram label, const State* state):
    _label(label), _ntokens(0), _ntables(0) {
    STATE = state;}
  Restaurant(Bigram label): _label(label), _ntokens(0), _ntables(0) {
    my_assert(STATE,*this);}
  virtual ~Restaurant() {}
  void check_invariant() const;
  bool empty() const {
    if (!_ntables) return true;
    return false;
  }
  const Bigram& label() const {return _label;}
  Count ntokens() const {return _ntokens;}
  //num of tokens at table i
  Count ntokens(Count i) const {
    Tables::const_iterator iter = _tables.find(i);
    if (iter == _tables.end())
      return 0;
    return iter->second;
  }
  Count ntables() const {return _ntables;}
  //add token to table i, return true if i is a new table
  // i can be up to (current final index + 1)
  // set unsafe->true only in log posterior when tables may be incr'd out of order.
  bool inc_table(Count i, bool unsafe=false);
  // removes a token, decrementing table count if needed.
  // returns true if table became empty
  bool dec_table(Count i);
  // old_table indicates a table whose count must be subtracted,
  // -1 if counts are all correct.
  Count sample_table(Float temp) const;
  friend ostream& operator<< (ostream& os, const Restaurant& r) {
    //    if (r.empty()) return os;
    os << r._label << " [ty=" << r._ntokens << ", to="
       << r._ntables << "]" << " " << r._tables
       << " free: " << r._free_list;
    return os;
  }
private:
  typedef Cs Indices;
  Bigram _label; // what bigram we correspond to
  Tables _tables; // number of tokens at each table
  Count _ntokens; // total number of tokens at all tables
  Count _ntables; // number of occupied tables
  Indices _free_list; // indices of free tables
  //  static Urn<Label, Float> _samples;
  const Tables& tables() const {return _tables;}
  // returns index of next free table
  Count next_empty() const {
    if (!_free_list.empty()) 
      return _free_list.back();
    return _ntables;
  }
};

#endif
