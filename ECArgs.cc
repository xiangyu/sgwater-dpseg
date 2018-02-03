#include "ECArgs.h"
#include <cstdio>
#include <set>
#include "utils.h"
#include <algorithm>

using namespace std;

ECArgs::
ECArgs(int argc, char *argv[], const string& argoptstr)
{
  set<char> argopts;
  for (unsigned int i = 0; i < argoptstr.length(); ++i)
    argopts.insert(argoptstr[i]);

  for(int i = 1 ; i < argc ; i++)
    {
      string arg(argv[i]);
      if(arg[0] != '-')
	{
	  argList.push_back(arg);
	}
      else
	{
	  nopts_++;
	  int l = arg.length();
	  assert(l > 1);
	  char optch = arg[1];
	  optList.push_back(string(1, optch));
	  if (l == 2)
	  {
	    if (argopts.count(optch))
	    {
	      ++i;
	      if (i >= argc) error ("Option not followed by argument!");
	      arg = argv[i];
	      optList.push_back(arg);
	    }
	    else
	      optList.push_back("");
	  }
	  else
	    optList.push_back(string(arg,2,l-2));
	}
    }
}

bool
ECArgs::
isset(char c) const
{
  string sig = "";
  sig += c;
  for (size_t i=0; i<optList.size(); i+=2) {
    if (optList[i] == sig) return true;
  }
  return false;
}


string
ECArgs::
value(char c) const
{
  string sig;
  sig += c;
  size_t i;
  for (i=0; i<optList.size(); i+=2) {
    if (optList[i] == sig) 
      return optList[++i];
  }      
  cerr << "Looking for value of on-line argument " << sig << endl;
  error("could not find value");
  return "";
}


