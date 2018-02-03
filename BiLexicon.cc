#include "BiLexicon.h"
#include "State.h"

extern Count debug_level;

void
BiLexicon::check_invariant() const {
#ifndef NDEBUG
  Parent::check_invariant();
  _tables.check_invariant();
  Count total_tables = 0;
  Count total_tokens = 0;
  cforeach(BiLexicon, i, *this) {
    _restaurants.find(i->first) !=_restaurants.end();
  }
  cforeach(Restaurants, i, _restaurants) {
    find(i->first) != end();
    i->second.check_invariant();
    total_tables += i->second.ntables();
    total_tokens += i->second.ntokens();
  }
  assert(total_tables == ntables());
  assert(total_tokens == ntokens());
#endif
}

// add bigram to random table and return index of table
size_t 
BiLexicon::inc(const Bigram& pair, Float temp) {
  Count index = 0;
  Restaurants::iterator i = _restaurants.find(pair);
  if (i != _restaurants.end()) {
    index = i->second.sample_table(temp);
  }
  place(pair, index);
  return index;
}

// remove bigram from random table and return index of table
size_t 
BiLexicon::dec(const Bigram& pair, Float temp) {
  Restaurants::iterator i = _restaurants.find(pair);
  my_assert(i != _restaurants.end(), pair);
  Count index = i->second.sample_table(temp);
  remove(pair, index);
  return index;
}

//if "unsafe", doesn't do most assertion checks even when debugging
//mode is on. this is needed for computing log posterior, b/c
//tables must be incremented out of order.
bool 
BiLexicon::place(const Bigram& b, Count table, bool unsafe) {
  debug_output(900, "BiLexicon::place(): (bg, table) = ", BiC(b,table));
  Restaurants::iterator i;
  if (Parent::inc(b)) {
    i = _restaurants.insert(Restaurant_pair(b, Restaurant(b))).first;
  }
  else {
    i = _restaurants.find(b);
  }
  bool added_table = false;
  if (i->second.inc_table(table, unsafe)) {
    _tables.inc(b.second);
    added_table = true;
  }
  if (!unsafe) 
    check_invariant();
  return added_table;
}

bool 
BiLexicon::remove(const Bigram& b, Count table) {
  debug_output(900, "BiLexicon::remove(): (bg, table) = ", BiC(b,table));
  Parent::dec(b);
  Restaurants::iterator i;
  i = _restaurants.find(b);
  my_assert(i != _restaurants.end(), b);
  bool removed_table = false;
  if (i->second.dec_table(table)) {
    _tables.dec(b.second);
    removed_table = true;
  }
  check_invariant();
  return removed_table;
}

