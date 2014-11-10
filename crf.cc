#include <cassert>
#include <iostream>
#include "crf.h"
using namespace std;

// returns log(a * exp(x) + b * exp(y))
adouble log_sum_exp(adouble x, adouble y, adouble a = 1.0, adouble b = 1.0) {
  adouble m = max(x, y);
  return m + log(a * exp(x - m) + b * exp(y - m));
}

adouble log_sum_exp(vector<adouble> v) {
  if (v.size() == 0) {
    return -numeric_limits<double>::infinity();
  }
  adouble m = v[0];
  for (int i = 1; i < v.size(); ++i) {
    if (v[i] > m) {
      m = v[i];
    }
  }

  adouble sum = 0.0;
  for (int i = 0; i < v.size(); ++i) {
    sum += exp(v[i] - m);
  }
  return m + log(sum);
}

crf::crf(adept::Stack* stack, feature_scorer* scorer) {
  this->stack = stack;
  this->scorer = scorer;
}

adouble crf::dot(const map<string, double>& features, const map<string, adouble>& weights) {
  adouble score = 0.0;
  for (auto& kvp : features) {
    if (weights.find(kvp.first) == weights.end()) {
      cerr << "ERROR: Invalid attempt to use unknown feature \"" << kvp.first << "\"." << endl;
    }
    assert(weights.find(kvp.first) != weights.end());
    auto x = weights.find(kvp.first);
    score += x->second * kvp.second;
  }
  return score;
}

adouble crf::score(const vector<string>& x, const Derivation& y) {
  map<string, double> features = scorer->score(x, y);
  return dot(features, weights);
}

// Computes log of sum_t sum_s exp (score(t|w) + score(s|t))
// where w is the source word
// t is a translation of w
// and s is a suffix on t
// Does NOT handle th ecase where w translates into NULL.
adouble crf::word_partition_function(const string& source) {
  vector<adouble> translation_scores;
  for (auto kvp : scorer->fwd_ttable->getTranslations(source)) {
    string target = kvp.first;
    map<string, double> translation_features = scorer->score_translation(source, target);

    vector<adouble> suffix_scores;
    for (string suffix : scorer->suffix_list) {
      map<string, double> suffix_features = scorer->score_suffix(target, suffix);
      adouble suffix_score = dot(suffix_features, weights);
      suffix_scores.push_back(suffix_score);
    }
    adouble suffix_scores_sum = log_sum_exp(suffix_scores);
    adouble translation_score = dot(translation_features, weights);

    translation_scores.push_back(translation_score + suffix_scores_sum);
  }
  return log_sum_exp(translation_scores);
}

adouble crf::partition_function(const vector<string>& x) {
  adouble z = 0.0;
  vector<adouble> non_null_scores;
  vector<adouble> null_scores;
  for (int i = 0; i < x.size(); ++i) {
    const string& source = x[i];
    adouble non_null_score = word_partition_function(source);
    adouble null_score;

    // Handle the case where the ith source word translates into NULL
    {
      map<string, double> translation_features = scorer->score_translation(source, "");
      map<string, double> suffix_features = scorer->score_suffix("", "");
      adouble translation_score = dot(translation_features, weights);
      adouble suffix_score = dot(suffix_features, weights);
      null_score = translation_score + suffix_score;
    }

    non_null_scores.push_back(non_null_score);
    null_scores.push_back(null_score);
  }

  assert(null_scores.size() == x.size());
  assert(non_null_scores.size() == x.size());
  assert(x.size() <= 5);

  vector<adouble> final_scores;
  const int factorial[] = {1, 1, 2, 6, 24, 120};
  for (unsigned int i = 0; i < (1 << x.size()); ++i) {
    adouble score = 0.0;
    for (int j = 0; j < x.size(); ++j) {
      if (i & (1 << j)) {
        score += non_null_scores[j];
      }
      else {
        score += null_scores[j];
      }
    }
    assert(popCount(i) <= 5);
    score += log(factorial[popCount(i)]);
    final_scores.push_back(score);
  }

  return log_sum_exp(final_scores);
}

adouble crf::slow_partition_function(const vector<string>& x, const map<string, adouble>& weights) {
  vector<vector<string> > candidate_translations;
  for (int i = 0; i < x.size(); ++i) {
    vector<string> translations;
    translations.push_back("");
    for (auto& kvp : scorer->fwd_ttable->getTranslations(x[i])) {
      translations.push_back(kvp.first);
    }
    candidate_translations.push_back(translations);
  }
  assert(candidate_translations.size() == x.size());


  vector<Derivation> derivations;
  // Loop over the cross product of possible translations
  for (vector<string> translations : cross(candidate_translations)) {
    // This variable will hold a permutation of the integers [0, |G|)
    // Note that we remove indices coresponding to NULL translations
    // since their ordering does not affect the output.
    vector<int> indices;
    for (int i = 0; i < translations.size(); ++i) {
      if (translations[i].size() != 0) {
        indices.push_back(i);
      }
    }

    // Don't allow all the pieces to translate as NULL
    /*if (indices.size() == 0) {
      continue;
    }*/

    vector<vector<string> > candidate_suffixes;
    for (int i = 0; indices.size() > 0 && i < indices.size() - 1; ++i) {
      vector<string> suffixes;
      for (string suffix : scorer->suffix_list) {
        suffixes.push_back(suffix);
      }
      candidate_suffixes.push_back(suffixes);
    }

    // The last iteration is split since eventually the last suffix
    // will be drawn from a different table
    if (indices.size() > 0)
    {
      vector<string> suffixes; 
      for (string suffix : scorer->suffix_list) {
        suffixes.push_back(suffix);
      }
      candidate_suffixes.push_back(suffixes);
    }
    assert(candidate_suffixes.size() == indices.size());

    // Loop over all possible permutations.
    do { 
      for (vector<string> chosen_suffixes : cross(candidate_suffixes)) {
        vector<string> suffixes(translations.size(), string(""));
        for (int i = 0; i < indices.size(); ++i) {
          suffixes[indices[i]] = chosen_suffixes[i];
        } 

        assert(suffixes.size() == translations.size());
        Derivation derivation { translations, suffixes, indices };
        derivations.push_back(derivation);
      }
    } while (next_permutation(indices.begin(), indices.end()));
  }

  vector<adouble> scores;
  for (Derivation d : derivations) {
    map<string, double> features = scorer->score(x, d);
    scores.push_back(dot(features, weights)); 
  }

  return log_sum_exp(scores);
}

adouble crf::score_noise(const vector<string>& x, const Derivation& y) {
  adouble score = 0.0;
  assert(x.size() == y.translations.size());
  for (int i = 0; i < x.size(); ++i) {
    double s;
    bool found = scorer->fwd_ttable->getScore(x[i], y.translations[i], s);
    s = found ? s : -10.0;
    score += s;
  }
  return score;
}

adouble crf::nce_loss(const vector<string>& x, const Derivation& y, const vector<Derivation>& n) {
  int k = n.size();
  adouble loss = 0.0;

  adouble py = score(x, y); // log u_model(x, y)
  adouble ny = score_noise(x, y); // log u_noise(x, y)
  assert(isfinite(py.value()));
  assert(isfinite(ny.value()));

  adouble pd1y = py - log_sum_exp(py, ny, 1.0, k); // log p(D = 1 | x, y)
  adouble pd0y = log(k) + ny - log_sum_exp(py, ny, 1.0, k); // log p(D = 0 | x, y)
  if (abs(exp(pd0y) + exp(pd1y) - 1.0) >= 0.0001) {
    cerr << "py = " << py << endl;
    cerr << "ny = " << py << endl;
    cerr << "pd0y = " << pd0y << endl;
    cerr << "pd1y = " << pd1y << endl;
  }
  assert(exp(pd1y) >= 0.0 && exp(pd1y) <= 1.0);
  assert(abs(exp(pd0y) + exp(pd1y) - 1.0) < 0.0001);

  loss += pd1y;

  for (int i = 0; i < k; ++i) {
    const Derivation& z = n[i]; // z is a noise sample
    adouble pz = score(x, z); // log u_model(x, z);
    adouble nz = score_noise(x, z); // log u_noise(x, z);
    adouble pd0z = log(k) + nz - log_sum_exp(pz, nz, 1.0, k); // p(D = 0 | x, y);
    adouble pd1z = pz - log_sum_exp(pz, nz, 1.0, k); // p(D = 1 | x, y);
    if (abs(exp(pd0z) + exp(pd1z) - 1.0) >= 0.0001) {
      cerr << "pz = " << pz << endl;
      cerr << "nz = " << pz << endl;
      cerr << "pd0z = " << pd0z << endl;
      cerr << "pd1z = " << pd1z << endl;
    }
    assert(exp(pd0z) >= 0.0 && exp(pd1y) <= 1.0);
    assert(abs(exp(pd0z) + exp(pd1z) - 1.0) < 0.0001);
    loss += pd0z;
  }
  return loss;
}

adouble crf::l2penalty(const double lambda) {
  adouble res = 0.0;
  for (auto& kvp : weights)
    res += lambda * kvp.second * kvp.second;
  return res;
}

adouble crf::train(const vector<vector<string> >& x, const vector<Derivation>& y,
    double learning_rate, double l2_strength) {
  assert(x.size() == y.size());
  stack->new_recording();
  adouble log_loss = 0.0;
  for (unsigned i = 0; i < x.size(); ++i) {
    adouble n = score(x[i], y[i]);
    adouble d = partition_function(x[i]);
    if (n >= d) {
      cerr << "n = " << n << endl;
      cerr << "d = " << d << endl;
    }
    assert(n < d);
    log_loss -= n - d;
  }
  log_loss += l2penalty(l2_strength);

  log_loss.set_gradient(1.0);
  stack->compute_adjoint();
  for (auto& fv : weights) {
    const double g = fv.second.get_gradient();
    //historical_gradients[fv.first] = rho * historical_gradients[fv.first] + (1 - rho) * g * g;
    //double delta = -g * sqrt(historical_deltas[fv.first]) / sqrt(historical_gradients[fv.first]);
    //historical_deltas[fv.first] = rho * historical_deltas[fv.first] + (1 - rho) * delta * delta;
    double delta = -g * learning_rate;
    weights[fv.first] += delta;
  }

  return log_loss;
}

adouble crf::train(const vector<vector<string> >& x, const vector<Derivation>& y,
    const vector<vector<Derivation> >& noise_samples, double learning_rate, double l2_strength) {
  assert(x.size() == y.size());
  stack->new_recording();
  adouble log_loss = 0.0;
  for (unsigned i = 0; i < x.size(); ++i) {
    log_loss += nce_loss(x[i], y[i], noise_samples[i]);
  }
  log_loss += l2penalty(l2_strength);

  log_loss.set_gradient(1.0);
  stack->compute_adjoint();
  for (auto& fv : weights) {
    const double g = fv.second.get_gradient();
    historical_gradients[fv.first] = rho * historical_gradients[fv.first] + (1 - rho) * g * g;
    double delta = -g * sqrt(historical_deltas[fv.first] + epsilon) / sqrt(historical_gradients[fv.first] + epsilon);
    historical_deltas[fv.first] = rho * historical_deltas[fv.first] + (1 - rho) * delta * delta;
    //double delta = -g * learning_rate;
    weights[fv.first] += delta;
  }

  return log_loss;
}

void crf::add_feature(string name) {
  if (weights.find(name) == weights.end()) {
    weights[name] = 0.0;
    historical_deltas[name] = 1.0;
    historical_gradients[name] = 1.0;
  }
}

vector<tuple<double, Derivation> > crf::predict(const vector<string>& x, int k) {
  scorer->suffix_list.insert("");
  // First we find the k-best (translation, suffix) pairs for each index in x
  vector<vector<tuple<adouble, string, string> > > best_pieces;
  for (int i = 0; i < x.size(); ++i) {
    vector<tuple<adouble, string, string> > local_best_pieces;
    local_best_pieces.reserve(k + 1);

    string source = x[i];
    vector<string> translation_list;
    translation_list.push_back("");
    for (auto kvp : scorer->fwd_ttable->getTranslations(source)) {
      translation_list.push_back(kvp.first);
    }

    for (string target : translation_list) {
      map<string, double> translation_features = scorer->score_translation(source, target);
      adouble translation_score = dot(translation_features, weights);
      for (string suffix : scorer->suffix_list) {
        if (target.size() == 0 && suffix.size() != 0) {
          continue;
        }
        map<string, double> suffix_features = scorer->score_suffix(target, suffix);
        adouble suffix_score = dot(suffix_features, weights);
        adouble total_score = suffix_score + translation_score;
        if (local_best_pieces.size() < k ||
            total_score > get<0>(local_best_pieces[local_best_pieces.size() - 1])) {
          local_best_pieces.push_back(make_tuple(total_score, target, suffix));
          std::sort(local_best_pieces.rbegin(), local_best_pieces.rend());
          assert(local_best_pieces.size() <= k + 1);
          if (local_best_pieces.size() == k + 1) {
            local_best_pieces.pop_back();
          }
        }
      }
    }
    best_pieces.push_back(local_best_pieces);
  }
  assert(best_pieces.size() == x.size());

  vector<tuple<double, Derivation> > kbest;
   
  // Now that we have the k-best (translation, suffix) pairs for each index,
  // run cube pruning to find our final k-best
  vector<tuple<adouble, vector<int> > > candidates;
  vector<int> start(x.size(), 0);
  assert(start.size() == x.size()); 
  adouble start_score = 0.0;
  for (int i = 0; i < x.size(); ++i) {
    assert(best_pieces[i].size() > 0);
    start_score += get<0>(best_pieces[i][0]);
  }
  candidates.push_back(make_tuple(start_score, start));
 
  while (candidates.size() > 0 && kbest.size() < k) { 
    auto best_candidate = candidates[candidates.size() - 1];
    candidates.pop_back();
    adouble score = get<0>(best_candidate);
    vector<int> indices = get<1>(best_candidate);
    assert(indices.size() == x.size());

    Derivation d;
    assert(d.translations.size() == d.suffixes.size());
    for (int i = 0; i < indices.size(); ++i) {
      auto& piece = best_pieces[i][indices[i]];
      string& translation = get<1>(piece);
      string& suffix = get<2>(piece);
      d.translations.push_back(translation);
      d.suffixes.push_back(suffix);
      assert(d.translations.size() == d.suffixes.size());
      if (translation.size() > 0) {
        d.permutation.push_back(i);
      }
    }
    assert(d.translations.size() == d.suffixes.size());
    assert(d.translations.size() == x.size());
    kbest.push_back(make_tuple(score.value(), d));

    assert (indices.size() == x.size());
    for (int i = 0; i < indices.size(); ++i) {
      vector<int> new_indices(indices.begin(), indices.end());
      assert (new_indices.size() == indices.size());
      new_indices[i]++;
      assert (new_indices.size() == x.size());
      adouble new_score = score - get<0>(best_pieces[i][indices[i]])
          + get<0>(best_pieces[i][new_indices[i]]);
      candidates.push_back(make_tuple(new_score, new_indices));
    }
  }
  return kbest;
}