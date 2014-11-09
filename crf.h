#pragma once
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include "adept.h"
#include "utils.h"
#include "feature_scorer.h"
using std::string;
using std::vector;
using std::map;
using std::tuple;
using adept::adouble;

class crf {
public:
  crf(adept::Stack* stack, feature_scorer* scorer);
  adouble dot(const map<string, double>& features, const map<string, adouble>& weights);
  adouble score(const vector<string>& x, const Derivation& y);
  adouble partition_function(const vector<string>& x);
  adouble slow_partition_function(const vector<string>& x,
    const map<string, adouble>& weights);
  adouble score_noise(const vector<string>& x, const Derivation& y);
  adouble nce_loss(const vector<string>& x, const Derivation& y, const vector<Derivation>& n);

  adouble l2penalty(const double lambda);
  adouble train(const vector<vector<string>>& x, const vector<Derivation>& z, double learning_rate, double l2_strength);
  adouble train(const vector<vector<string>>& x, const vector<Derivation>& z, const vector<vector<Derivation> >& noise_samples, double learning_rate, double l2_strength);
  void add_feature(string name);

  tuple<Derivation, double> predict(const vector<string>& x);
  vector<tuple<Derivation, double> > predict(const vector<string>& x, int k);

//private:
  map<string, adouble> weights;

private:
  adept::Stack* stack;
  feature_scorer* scorer;
  map<string, double> historical_deltas;
  map<string, double> historical_gradients;
  const double rho = 0.95;
  const double epsilon = 1.0e-6;
};

