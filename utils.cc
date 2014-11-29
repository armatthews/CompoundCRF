#include <algorithm>
#include <cassert>
#include "utils.h"
using namespace std;

string to_lower_case(string s) {
  string t = s;
  transform(t.begin(), t.end(), t.begin(), ::tolower);
  return t;
}

vector<vector<string> > cross(vector<vector<string> > vectors ) {
  vector<vector<string> > cross_product;

  // There's exactly one permutation of nothing
  if (vectors.size() == 0) {
    vector<string> empty;
    cross_product.push_back(empty);
    return cross_product;
  }

  vector<unsigned> indices(vectors.size(), 0);
  while (true) {
    vector<string> item;
    item.reserve(vectors.size());
    for (unsigned i = 0; i < vectors.size(); ++i) {
      item.push_back(vectors[i][indices[i]]);
    }
    cross_product.push_back(item);

    for (unsigned i = vectors.size(); i > 0;) {
      --i;
      indices[i]++;
      if (indices[i] == vectors[i].size()) {
        if (i != 0) {
          indices[i] = 0;
        }
        else {
          return cross_product;
        }
      }
      else {
        break;
      }
    }
  }
}

unsigned int popCount(unsigned int i) {
  unsigned int r = 0;
  unsigned int one = 1;
  for (unsigned b = 0; b < 8 * sizeof(unsigned int); ++b) {
    unsigned int mask = one << b;
    if (i & mask) {
      ++r;
    }
  }
  return r;
}
