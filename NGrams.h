#ifndef _NGRAMS_H_
#define _NGRAMS_H_

struct Bigram {
  Bigram(const std::string& f, const std::string& s) : first(f), second(s) {}
  virtual ~Bigram() {}
  std::string first;
  std::string second;
  friend std::ostream& operator<< (std::ostream& os, const Bigram& bg) {
    os << "(" << bg.first << " " << bg.second << ")";
    return os;
  }
};

inline bool operator==(const Bigram& x, const Bigram& y) {
  if (x.first != y.first) return false;
  if (x.second != y.second) return false;
  return true;
}

inline bool operator<(const Bigram& x, const Bigram& y) {
  if (x.first < y.first) return true;
  if (y.first < x.first) return false;
  if (x.second < y.second) return true;
  return false;
}

//copied from Mark's pair hash
namespace std {
template <> struct hash<Bigram> {
  size_t operator()(const Bigram& b) const {
    size_t h1 = hash<std::string>()(b.first);
    size_t h2 = hash<std::string>()(b.second);
    return h1 ^ (h1 >> 1) ^ h2 ^ (h2 << 1);
  }
};
}

struct Trigram {
  Trigram(const std::string& f, const std::string& s, const std::string& t) : 
    first(f), second(s), third(t) {}
  virtual ~Trigram() {}
  friend std::ostream& operator<< (std::ostream& os, const Trigram& tg) {
    os << "(" << tg.first << " " << tg.second << " " << tg.third << ")";
    return os;
  }
  std::string first;
  std::string second;
  std::string third;
};

namespace std {
template <> struct hash<Trigram> {
  size_t operator()(const Trigram& t) const {
    size_t h1 = hash<std::string>()(t.first);
    size_t h2 = hash<std::string>()(t.second);
    size_t h3 = hash<std::string>()(t.third);
    size_t tmp = h1 ^ (h1 >> 1) ^ h2 ^ (h2 << 1);
    return tmp ^ (tmp >> 1) ^ h3 ^ (h3 << 1);
  }
};
}

inline bool operator==(const Trigram& x, const Trigram& y) {
  if (x.first != y.first) return false;
  if (x.second != y.second) return false;
  if (x.third != y.second) return false;
  return true;
}

inline bool operator<(const Trigram& x, const Trigram& y) {
  if (x.first < y.first) return true;
  if (y.first < x.first) return false;
  if (x.second < y.second) return true;
  if (y.second < x.second) return false;
  if (x.third < y.third) return true;
  return false;
}

#endif
