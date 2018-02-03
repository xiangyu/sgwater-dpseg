#include "Restaurant.h"
#include "State.h"
#include "Urn.h"
#include "utils.h"

extern Count debug_level;

const State* Restaurant::STATE = 0;
//Urn<Label, Float> Restaurant::_samples;

//1.  Number of tables and tokens is consistent
//2.  Max index of occupied tables <= # tables + free list size -1
//3.  Max index of (occupied tables and free list) 
//    equals (# tables + free list size -1)
//4.  Indices in free list are not used in occupied tables
//5.  Exactly all indices from 0 .. (#tables + free list size -1)
//    are either occupied or in free list.
void 
Restaurant::check_invariant() const {
#ifndef NDEBUG
  my_assert(_ntables == _tables.size(), CC(_ntables, _tables.size())); //1
  Count total = 0;
  if ((_ntables == 0)  && (_free_list.size() == 0))
    return;
  Bs ok(_ntables + _free_list.size() -1, false);
  Count max_index = 0;
  cforeach (Tables, t, _tables) {
      total += t->second;
      ok[t->first] = true;
      if (t->first > max_index) max_index = t->first;
    }
  my_assert(_ntokens == total, CC(_ntokens, total)); //1
  my_assert(max_index <= _ntables +_free_list.size()-1, *this); //2
  cforeach (Indices, i, _free_list) {
    my_assert(_tables.find(*i) == _tables.end(),*this); //4
    ok[*i] = true;
    if (*i > max_index) max_index = *i;
  }
  my_assert(max_index == _ntables +_free_list.size()-1, *this); //3
  for (Count i = 0; i < ok.size(); i++) { //5
    my_assert(ok[i], *this);
  }
#endif
}

//add token to table i, return true if i is a new table.
//if "unsafe", doesn't do most assertion checks even when debugging
//mode is on. this is needed for computing log posterior, b/c
//tables must be incremented out of order.
//DON'T USE "UNSAFE" OTHERWISE!
bool 
  Restaurant::inc_table(Count i, bool unsafe) {
  debug_output(1100, "Restaurant::inc_table() ", *this);
  if (!unsafe) {
    my_assert(i <= (_ntables + _free_list.size()), i); //note: some values that pass this may still be invalid.
    check_invariant();
  }
 if (!_free_list.empty() && (i == _free_list.back())) {
    my_assert(_tables.find(i) == _tables.end(), *(_tables.find(i)));
    _tables.insert(CC(i,0));
    _free_list.pop_back();
 }  
 else if (i == _ntables) {
    _tables.insert(CC(i, 0));
 }
  _tables[i]++;
  _ntokens++;
  if (_tables[i] == 1) {
    _ntables++;
    debug_output(1200, "after Restaurant::inc_table(1) ", *this);
    return true;
  }
  debug_output(1200, "after Restaurant::inc_table(2) ", *this);
  return false;
}


// removes a token, decrementing table count if needed.
// returns true if table became empty
bool 
Restaurant::dec_table(Count i) {
  debug_output(1100, "Restaurant::dec_table() ", *this);
  my_assert(_tables.find(i) != _tables.end(), i);
  _tables[i]--;
  _ntokens--;
  if (_tables[i] == 0) {
    _ntables--;
    _tables.erase(i);
    _free_list.push_back(i);
    debug_output(1200, "after Restaurant::dec_table(1) ", *this);
    return true;
  }
  debug_output(1200, "after Restaurant::dec_table(2) ", *this);
  return false;
}

// returns the index of a table according to CRP distrib.
// index is between 0 and _ntables, where the latter
// indicates a new table.
Count
Restaurant::sample_table(Float temp) const {
  //  cout << "random" << endl;
  check_invariant();
  if (_ntables == 0)
    return next_empty();
  Urn<Count, Float> new_tables;
  new_tables.reserve(_ntables+1);
  cforeach (Tables, t, _tables) {
    new_tables.push(t->first,pow(t->second,temp));
  }
  CF p(next_empty(),pow(STATE->p_word(_label),temp));
  new_tables.push(p);
  debug_output(700, "Restaurant::random_table() weights ", new_tables);
  Count t = new_tables.draw();
  return t;
}

