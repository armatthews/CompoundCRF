#include <vector>
#include <string>
#include "ttable.h"
#include "utils.h"
using std::string;
using std::vector;

class compound_analyzer {
public:
  compound_analyzer(ttable* fwd_ttable);
  bool decompose(string compound, const vector<string>& pieces,
    vector<int> permutation, vector<string>& suffixes);
  vector<Derivation> analyze(const vector<string>& english, string german, bool verbose = false);
private:
  ttable* fwd_ttable;
};
