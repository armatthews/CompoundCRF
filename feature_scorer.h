#pragma once
#include <unordered_set>
#include <map>
#include <vector>
#include <string>
#include "ttable.h"
#include "utils.h"
using std::string;
using std::vector;
using std::unordered_set;
using std::map;

class feature_scorer {
public:
  feature_scorer(ttable* fwd_ttable, ttable* rev_ttable);
  double lexical_score(ttable* table, const string& source,
    const string& target);
  double lexical_score(ttable* table, const vector<string>& source,
    const vector<string>& target, const vector<int>& permutation);

  map<string, double> score_translation(const string& source,
    const string& target);
  map<string, double> score_suffix(const string& root, const string& suffix);
  map<string, double> score_permutation(const vector<string>& source,
    const vector<int>& permutation);

  map<string, double> score(const vector<string>& source,
    const Derivation& derivation);

  double oov_score = -10.0;
//private:
  ttable* fwd_ttable;
  ttable* rev_ttable;
  unordered_set<string> suffix_list;
};
