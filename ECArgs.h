
#ifndef ECARGS_H
#define ECARGS_H

#include <list>
#include <string>
#include <vector>

class ECArgs
{
 public:
  ECArgs(int argc, char *argv[], const std::string& argopts = "");
  int nargs() const { return argList.size(); }
  bool isset(char c) const;
  std::string value(char c) const;
  std::string arg(int n) const { return argList[n]; }
 private:
  int nargs_;
  int nopts_;
  std::vector<std::string> argList;
  std::vector<std::string> optList;
};
  

#endif /* ! ECARGS_H */
