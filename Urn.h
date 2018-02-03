// urn.h
//
// Mark Johnson, 6th March 2005
// Modified by sgwater, 3/10/05
//
// An urn holds a sequence of objects, each of which is
// associated with some weight.
//
// You create an urn object, set ts to contain a set of
// objects and weights, and then call sum_weights().
//
#ifndef URN_H
#define URN_H

#include <cstdlib>
#include <vector>
#include <cassert>
#include "utils.h"

//! An urn simulates random draws of objects from a set of objects, each of
//! which has a positive weight.
//!
//! To use an urn, set ts to the (object,weight) pairs, call sum_weights() and
//! then call draw().
//
template <typename object_type, typename weight_type> 
class Urn {

private:

  struct tuple {
    object_type object;       //!< object in the urn, set by user
    weight_type weight;       //!< numerical weight of object, set by user
    weight_type sum_weight;   //!< sum of descendant's weights, set by sum_weights()

    tuple(const object_type& o, const weight_type& w = weight_type())
      : object(o), weight(w), sum_weight(0) 
    { }  // urn::tuple::tuple()
  };  // urn::tuple{}

  typedef double Float;
  typedef std::vector<tuple> tuples;
  tuples ts;

  //! uniform() returns a random number in [0,1)
  //
  Float uniform() const {
    return std::rand()/(RAND_MAX+1.0);
  }  // urn::uniform

public:

  Urn() { }
  Urn(size_t n) : ts(n) { }

  size_t size() { return ts.size(); }

  void resize(size_t n) { ts.resize(n); }

  //! push() adds a new item to the urn
  //
  void push(std::pair<object_type, weight_type> p) {
    push(p.first, p.second);
  }
  void push(const object_type& object, const weight_type& weight=1) {
    tuple t(object, weight);
    if (ts.empty()) {
      t.sum_weight = weight;
    }
    else {
      t.sum_weight = ts.back().sum_weight + weight;
    }
    ts.push_back(t);
  }  // urn::push()

  //! reserve() reserves space for n objects (but does not create them)
  //
  void reserve(size_t n) { ts.reserve(n); }

  //! draw() returns an object randomly drawn from the urn
  //
  object_type& draw() {
    assert(ts.size() > 0);
    Float weight = ts.back().sum_weight*uniform();
    //    if (DEBUG > 100)
    //      std::cerr << "weight is " << weight << std::endl;
    assert(weight < ts.back().sum_weight);
    if (weight < ts[0].weight)
      return ts[0].object;
    return ts[draw(0, ts.size()-1, weight)].object;
  }  // urn::draw()

  const weight_type& sum_weights() {
    return ts.back().sum_weight;
  }

  friend ostream& operator<< (ostream& os, const Urn<object_type, weight_type>& u) {
    typename Urn<object_type, weight_type>::tuples::const_iterator t;
    for (t = u.ts.begin(); t != u.ts.end(); t++) {
      os << "(" << t->object << "," << t->weight << ") ";
    }
    os << endl;
    return os;
  }
private:

  // do a binary search to find the object whose total
  // weight is just > weight.  Returns object index.
  size_t draw(size_t lower, size_t upper, Float weight) const {
    my_assert(lower >= 0, lower);
    my_assert(upper < ts.size(), upper);
    if (upper - lower == 1) {
      my_assert(ts[lower].sum_weight <= weight, ts[lower].sum_weight);
      my_assert(ts[upper].sum_weight > weight, ts[upper].sum_weight);
      //      std::cout << ts[lower].sum_weight << " " << ts[upper].sum_weight << std::endl;
      return upper;
    }
    else {
      size_t middle = (upper - lower)/2 + lower;
      if (ts[middle].sum_weight > weight) {
	return draw(lower, middle, weight);
      }
      else {
	return draw(middle, upper, weight);
      }
    }
  }  // urn::draw()
	
}; // urn{}

#endif // URN_H
