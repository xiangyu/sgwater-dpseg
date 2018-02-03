#ifndef _DATAFILE_H_
#define _DATAFILE_H_

#include <fstream>
#include <iostream>
#include "Utterance.h"

using std::string;
using std::vector;

// exception classes
class FileError {};
class UtteranceLength {
public:
  UtteranceLength(const std::string& u):_u(u) {};
  void print() {cout << "Error: Utterance is too long:" << endl
		     << _u << endl 
		     << "(Increase Utterance::MAX_LENGTH)" << endl;}
private:
  string _u;
};



class DatafileBase {
public:
  DatafileBase() {};
  virtual ~DatafileBase() {}
  // return the next reference transcription in some order.
  // each transcription in file will be returned exactly once
  // returns empty string when no more transcriptions.
  virtual string next_reference() = 0;//throw(UtteranceLength) = 0;
  // reset to a state as if you had created a new Datafile
  virtual void reset() = 0;
};

class Datafile: public DatafileBase {
public:
  Datafile(const string& filename) throw(FileError);
  virtual ~Datafile() {}
  // get next reference transcription in order from file
  virtual string next_reference() throw(UtteranceLength);
  virtual void reset();
private:
  ifstream _file;
  const char* _filename;
};

class RepeatDatafile: public DatafileBase {
public:
  RepeatDatafile(const string& filename) throw(UtteranceLength,
					       FileError);
  virtual ~RepeatDatafile() {}
  virtual string next_reference();
  virtual void reset();
private:
  vector<string> _utterances;
  vector<string>::iterator _current;
};

class RandomDatafile: public DatafileBase {
public:
  RandomDatafile(const string& filename) throw(UtteranceLength,
					       FileError);
  RandomDatafile(const string& filename, long seed) throw(UtteranceLength,
					       FileError);
  virtual ~RandomDatafile() {}
  long seed() {return _seed;}
  const string& seed_type() {return _seed_type;}
  // get next reference transcription in random order
  virtual string next_reference();// throw(UtteranceLength);
  virtual void reset();
private:
  vector<string> _utterances;
  vector<string>::iterator _current; // current utterance
  long _seed;
  string _seed_type;
};

#endif
