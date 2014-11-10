#include <set>
#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <fstream>
#include <sstream>

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#include "adept.h"
#include "crf.h"
#include "feature_scorer.h"
#include "compound_analyzer.h"
#include "noise_model.h"

using namespace std;
using adept::adouble;

const double eta = 0.1;
const double lambda = 1.0;
const int num_noise_samples = 100;

void read_input_file(string filename, vector<vector<string> >& X, vector<string>& Y) {
  ifstream f(filename);
  if (!f.is_open()) {
    cerr << "Unable to read from file " << filename << "." << endl;
    exit(1);
  }

  string line;
  vector<string> source;
  while (getline(f, line)) {
    stringstream sstream(line);
    string word;
    while (sstream >> word) {
      source.push_back(to_lower_case(word));
    }
    if (source.size() > 0) {
      string target = source[source.size() - 1];
      source.pop_back();
      X.push_back(source);
      Y.push_back(target);
      source.clear();
    }
  }
}

Derivation sample_derivation(crf* model, const vector<string>& input, const vector<Derivation>& derivations) {
  vector<adouble> scores(derivations.size(), 0.0);
  adouble sum = 0.0;
  for (int i = 0; i < derivations.size(); ++i) {
    adouble score = model->score(input, derivations[i]);
    score = exp(score);
    scores[i] = score;
    sum += score; 
  }

  adouble r = sum * (double)rand() / RAND_MAX;
  for (int i = 0; i < derivations.size(); ++i) {
    if (r < scores[i]) {
      return derivations[i];
    }
    r -= scores[i];
  }

  assert(false);
}

vector<Derivation> sample_derivations(crf* model, const vector<vector<string> >& inputs, const vector<vector<Derivation> >& derivations) {
  assert (inputs.size() == derivations.size());

  vector<Derivation> sampled_derivations;
  for (int i = 0; i < inputs.size(); ++i) { 
    const Derivation& sample = sample_derivation(model, inputs[i], derivations[i]);
    sampled_derivations.push_back(sample);
  }
  assert (sampled_derivations.size() == inputs.size());
  return sampled_derivations;
}

void exception_handler(int sig) {
  void *array[10];
  int size = backtrace(array, 10);

  cerr << "Error: signal " << sig << ":" << endl;
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

void test(int argc, char** argv) {
  ttable fwd_ttable;
  ttable rev_ttable;
  fwd_ttable.load(argv[2]);
  rev_ttable.load(argv[3]);
  feature_scorer scorer(&fwd_ttable, &rev_ttable);
  compound_analyzer analyzer(&fwd_ttable);

  adept::Stack stack;
  crf model(&stack, &scorer);
  model.add_feature("fwd_score");
  model.add_feature("rev_score");
  model.add_feature("tgt_null");
  model.add_feature("suffix_n"); 
  model.add_feature("suffix_");
  scorer.suffix_list.insert("");
  scorer.suffix_list.insert("n");

  vector<string> input {"tomato", "processing"}; 

  cerr << "Computing fast partition function..." << endl;
  adouble fast = model.partition_function(input);
  cerr << "Fast partition function: " << fast << endl;

  cerr << "Computing slow partition function..." << endl;
  adouble slow = model.slow_partition_function(input, model.weights);
  cerr << "Slow partition function: " << slow << endl;

  double diff = abs(fast.value() - slow.value());
  assert (diff < 1.0e-6);
}

int main(int argc, char** argv) {
  signal(SIGSEGV, exception_handler);
  if (argc != 4) {
    cerr << "Usage: " << argv[0] << " train.txt fwd_ttable rev_ttable\n";
    return 1;
  }

  // Quick sanity check
  test(argc, argv);

  // read training data
  vector<vector<string> > train_source;
  vector<string> train_target;
  vector<vector<Derivation> > train_derivations;
  read_input_file(argv[1], train_source, train_target);

  cerr << "Successfully read " << train_source.size() << " training instances." << endl;
  assert (train_source.size() == train_target.size());

  // Read in the ttables
  cerr << "Loading ttables..." << endl;
  ttable fwd_ttable;
  ttable rev_ttable;
  fwd_ttable.load(argv[2]);
  rev_ttable.load(argv[3]);
  feature_scorer scorer(&fwd_ttable, &rev_ttable);
  compound_analyzer analyzer(&fwd_ttable);

  // Analyze the target side of the training corpus into lists of possible derivations
  cerr << "Analyzing training data..." << endl;
  for (int i = 0; i < train_source.size(); ++i) {
    vector<Derivation> derivations = analyzer.analyze(train_source[i], train_target[i]);
    train_derivations.push_back(derivations);
  }

  // Remove any unreachable references from the training data
  for (int i = 0; i < train_source.size(); ++i) {
    if (train_derivations[i].size() == 0) { 
      train_source.erase(train_source.begin() + i);
      train_target.erase(train_target.begin() + i); 
      train_derivations.erase(train_derivations.begin() + i);
      i--;
    }
  }

  adept::Stack stack;
  crf model(&stack, &scorer);
  noise_model noise_generator(&fwd_ttable);

  // Preload features into the CRF to avoid adept errors
  for (int i = 0; i < train_source.size(); ++i) {
    for (Derivation& derivation : train_derivations[i]) {
      for (string suffix : derivation.suffixes) {
        scorer.suffix_list.insert(suffix);
      }
      map<string, double> features = scorer.score(train_source[i], derivation);
      for (auto& kvp : features) {
        model.add_feature(kvp.first);
      }
    }
  }

  vector<vector<Derivation> > noise_samples; 
  for (int i = 0; i < train_source.size(); ++i) {
    vector<Derivation> samples;
    for (int j = 0; j < num_noise_samples; ++j) {
      Derivation sample = noise_generator.sample(train_source[i]);
      samples.push_back(sample);
    }
    noise_samples.push_back(samples);
  }

  assert (train_source.size() == train_target.size());
  assert (train_source.size() == train_derivations.size());
  cerr << train_source.size() << " reachable examples remain." << endl;

  adouble loss;
  vector<Derivation> chosen_derivations = sample_derivations(&model, train_source, train_derivations);

  loss = 0.0;
  for (unsigned i = 0; i < train_source.size(); ++i) {
    loss += model.nce_loss(train_source[i], chosen_derivations[i], noise_samples[i]);
  }
  loss += model.l2penalty(lambda);
  cerr << "Iteration " << 0 << " loss: " << loss << endl;

  for (unsigned iter = 0; iter < 100; ++iter) {
    //loss = model.train(train_source, chosen_derivations, noise_samples, eta, lambda);
    loss = model.train(train_source, chosen_derivations, eta, lambda);
    cerr << "Iteration " << iter + 1 << " loss: " << loss << endl;
  }

  cerr << "Final loss: " << loss << endl;
  cerr << "Final weights: " << endl;
  for (auto kvp : model.weights) {
    if (abs(kvp.second) > 1.0e-10) {
      cerr << "  " << kvp.first << ": " << kvp.second << endl;
    }
  }

  vector<tuple<double, Derivation> > kbest = model.predict(train_source[0], 10);
  cerr << "Got " << kbest.size() << " best hypotheses:" << endl;
  for (int i = 0; i < kbest.size(); ++i) {
    cout << i << " ||| " << get<1>(kbest[i]).toLongString() << " ||| " << get<0>(kbest[i]) << endl;
  }
}
