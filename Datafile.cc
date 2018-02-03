#include <algorithm>
#include "Datafile.h"
#include "utils.h"

using namespace std;

Datafile::Datafile(const string& filename) throw(FileError)
  : _file(filename.c_str()), _filename(filename.c_str())
{
  if (!_file) throw FileError();
}

/* skips blank lines, returning next reference
transcription.  Removes spaces, but inserts SENTINEL 
character after each word.  
Returns empty string at eof. */
string
Datafile::next_reference() throw(UtteranceLength)
{
  if (!_file) {
    return string();
  }

  char buffer[Utterance::MAX_LENGTH];
  char c;
  int index = 0;
  
  // ignore empty lines
  while (_file.get(c) && c  == '\n') {
  }
  _file.putback(c);
  while (_file.get(c) && c != '\n') {
    if (index == Utterance::MAX_LENGTH) {
      throw UtteranceLength(buffer);
    }
    if (c == ' ') {
      buffer[index] = SENTINEL;
    }
    else {
      buffer[index] = c;
    }
    index++;
  }
  // happens at end of file
  if (index == 0) {
    return string();
  }
  buffer[index++] = SENTINEL;
  buffer[index] = 0;
  return buffer;
}

void
Datafile::reset()
{
  error("Datafile::reset doesn't work; fix it!");
  _file.close();
  _file.open(_filename);
}

RepeatDatafile::RepeatDatafile(const string& filename)
  throw (UtteranceLength, FileError)
{
  Datafile file(filename);
  string utterance = file.next_reference();
  while (!utterance.empty()) {
    _utterances.push_back(utterance);
    utterance = file.next_reference();
  }
  _current = _utterances.begin();
}

string
RepeatDatafile::next_reference()
{
  if (_current == _utterances.end()) return string();
  return *_current++;
}

void
RepeatDatafile::reset()
{
  _current = _utterances.begin();
}

RandomDatafile::RandomDatafile(const string& filename) throw(UtteranceLength, FileError)
{
  Datafile file(filename);
  string utterance = file.next_reference();
  while (!utterance.empty()) {
    _utterances.push_back(utterance);
    utterance = file.next_reference();
  }
  // The stl uses lrand48() if possible, otherwise
  // rand(), so we need to seed the correct one.
_seed = time(0);
#ifdef OS_WINDOWS
  srand(_seed);
  _seed_type = "Cygwin";
#else      
  srand48(_seed);
  _seed_type = "Linux";
#endif
  random_shuffle(_utterances.begin(), _utterances.end());
  _current = _utterances.begin();
}

RandomDatafile::RandomDatafile(const string& filename, long seed) throw(UtteranceLength, FileError)
{
  Datafile file(filename);
  string utterance = file.next_reference();
  while (!utterance.empty()) {
    _utterances.push_back(utterance);
    utterance = file.next_reference();
  }
  // The stl uses lrand48() if possible, otherwise
  // rand(), so we need to seed the correct one.
  _seed = seed;
#ifdef OS_WINDOWS
  srand(_seed);
  _seed_type = "Cygwin";
#else      
  srand48(_seed);
  _seed_type = "Linux";
#endif
  random_shuffle(_utterances.begin(), _utterances.end());
  _current = _utterances.begin();
}

string
RandomDatafile::next_reference() // throw(UtteranceLength)
{
  if (_current == _utterances.end()) return string();
  return *_current++;
}

void
RandomDatafile::reset()
{
  _current = _utterances.begin();
  /*
  _seed = clock();
#ifdef OS_WINDOWS
  srand(_seed);
  _seed_type = "Cygwin";
#else      
  srand48(_seed);
  _seed_type = "Linux";
#endif
  random_shuffle(_utterances.begin(), _utterances.end());
  _current = _utterances.begin();
  */
}
