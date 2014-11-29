#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <cassert>
#include "compound_analyzer.h"
#include "feature_scorer.h"
#include "utils.h"
#include "derivation.h"
using namespace std;

void process(const vector<string>& english, string german,
    compound_analyzer* analyzer, feature_scorer* scorer) {

  for (Derivation& derivation : analyzer->analyze(english, german, true)) {
    vector<string>& translations = derivation.translations;
    vector<string>& suffixes = derivation.suffixes;
    vector<int>& indices = derivation.permutation;

    // Output the translations and suffixes
    for (unsigned i = 0; i < indices.size(); ++i) {
      cerr << translations[indices[i]] << "+" << suffixes[i] << " ";
    }
    cerr << "||| ";

    // Output the permutation
    for (unsigned i = 0; i < indices.size(); ++i) {
      cerr << indices[i] << " ";
    }
    cerr << "||| ";

    // Output the features
    map<string, double> features = scorer->score(english, derivation);
    for (auto it = features.begin(); it != features.end(); ++it) {
      cerr << it->first << "=" << it->second << " ";
    }
    cerr << endl;
  }
}

void ShowUsageAndExit(char** argv) {
  cerr << "Usage: " << argv[0] << " fwd_ttable rev_ttable" << endl;
  exit(1);
}

int main(int argc, char** argv) {
  if (argc < 3) {
    ShowUsageAndExit(argv);
  }
  ttable fwd_ttable;
  ttable rev_ttable;
  fwd_ttable.load(argv[1]);
  rev_ttable.load(argv[2]);
  feature_scorer scorer(&fwd_ttable, &rev_ttable);
  compound_analyzer analyzer(&fwd_ttable);

  string line;
  while (getline(cin, line)) {
    stringstream sstream(line);
    vector<string> english;
    string german;
    string temp;
    while (sstream >> temp) {
      temp = to_lower_case(temp);
      english.push_back(temp);
    }
    german = english[english.size() - 1];
    english.pop_back();

    process(english, german, &analyzer, &scorer);
  }

  return 0;
}
