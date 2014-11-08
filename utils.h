#pragma once
#include <string>
#include <vector>

std::string to_lower_case(std::string s);
std::vector<std::vector<std::string> > cross(std::vector<std::vector<std::string> > vectors);

struct Derivation {
  std::vector<std::string> translations;
  std::vector<std::string> suffixes;
  std::vector<int> permutation;

  std::string toString();
};

