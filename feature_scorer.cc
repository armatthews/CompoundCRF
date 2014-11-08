#include <cassert>
#include <iostream>
#include "feature_scorer.h"
using namespace std;

feature_scorer::feature_scorer(ttable* fwd, ttable* rev) {
  fwd_ttable = fwd;
  rev_ttable = rev;
}

double feature_scorer::lexical_score(ttable* table, const string& source,
  const string& target) {
  if (source.size() == 0 || target.size() == 0) {
    return 0.0;
  }

  double score;
  if (table->getScore(source, target, score)) {
    return score;
  }
  else {
    return oov_score;
  }
}

double feature_scorer::lexical_score(ttable* table, const vector<string>& source,
    const vector<string>& target, const vector<int>& permutation) {
  double total_score = 0.0; 
  for (int i : permutation) {
    total_score += lexical_score(table, source[i], target[i]);
  }
  return total_score;
}

map<string, double> feature_scorer::score_translation(const string& source,
    const string& target) {
  map<string, double> features;
  features["tgt_null"] = (target.size() == 0) ? 1 : 0;
  features["fwd_score"] = lexical_score(fwd_ttable, source, target);
  features["rev_score"] = lexical_score(rev_ttable, target, source);
  return features;
}

map<string, double> feature_scorer::score_suffix(const string& root, const string& suffix) {
  suffix_list.insert(suffix);
  map<string, double> features;
  features["suffix_" + suffix] = 1.0;
  return features;
}

map<string, double> feature_scorer::score_permutation(const vector<std::string>& source,
    const vector<int>& permutation) {
  map<string, double> features;
  return features;
}

map<string, double> feature_scorer::score(const vector<string>& source,
    const Derivation& derivation) {
  const vector<string> translations = derivation.translations;
  const vector<string> suffixes = derivation.suffixes;
  const vector<int> permutation = derivation.permutation;

  assert (source.size() == translations.size());
  assert (translations.size() == suffixes.size());
  assert (permutation.size() <= translations.size());

  map<string, double> features;

  for (auto& kvp : score_permutation(source, permutation)) {
    features[kvp.first] += kvp.second;
  }

  for (int i = 0; i < translations.size(); ++i) {
    for (auto& kvp : score_translation(source[i], translations[i])) {
      features[kvp.first] += kvp.second;
    }

    for (auto& kvp : score_suffix(translations[i], suffixes[i])) {
      features[kvp.first] += kvp.second;
    }
  }

  assert(features["tgt_null"] == source.size() - permutation.size());

  double fwd_score = lexical_score(fwd_ttable, source, translations, permutation);
  assert(abs(features["fwd_score"] - fwd_score) < 0.00001);

  double rev_score = lexical_score(rev_ttable, translations, source, permutation);
  assert(abs(features["rev_score"] - rev_score) < 0.00001);

  return features;
}

