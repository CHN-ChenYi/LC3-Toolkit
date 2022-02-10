
#include <algorithm>
#include <bitset>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using Address = int16_t;

std::vector<std::pair<std::string, Address>> code;
std::unordered_map<std::string, Address> label;

void ToLower(std::string &s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

template <int n>
void BinaryPrint(int x) {
  if (x >= 0) {
    std::bitset<n> x_(x);
    std::cout << x_;
  } else {
    std::bitset<n> x_(x + (1 << n));
    std::cout << x_;
  }
}

template <int n>
int GetImm(const std::string &s, const std::string &token = "",
           const bool print = true) {
  int x;
  auto parsed = std::regex_replace(
      s, std::regex(".*" + token + "\\s*[#|x](-?[\\da-fA-F]+).*"), "$1");
  std::stringstream ss(parsed);
  ss >> (s.find('x') != std::string::npos ? std::hex : std::dec) >> x;
  if (print) BinaryPrint<n>(x);
  return x;
}

std::string GetOneReg(const std::string &s, const std::string &token) {
  auto parsed = std::regex_replace(
      s, std::regex(".*" + token + "\\s*r([0-7])\\s*(.*)"), "$1 $2");
  std::stringstream ss(parsed);
  int x;
  ss >> x;
  std::getline(ss, parsed);
  BinaryPrint<3>(x);
  return parsed;
}

std::string GetTwoReg(const std::string &s, const std::string &token) {
  auto parsed = std::regex_replace(
      s, std::regex(".*" + token + "\\s*r([0-7])\\s*,\\s*r([0-7])\\s*(.*)"),
      "$1 $2 $3");
  std::stringstream ss(parsed);
  int x, y;
  ss >> x >> y;
  std::getline(ss, parsed);
  BinaryPrint<3>(x);
  BinaryPrint<3>(y);
  return parsed;
}

template <int n>
void GetOffset(const std::string &s, const std::string &token,
               const int &cur_offset) {
  if (s.find('#') != std::string::npos) {
    GetImm<n>(s, token);
  } else {
    BinaryPrint<n>(
        label[std::regex_replace(
            s, std::regex(".*" + token + "\\s*([A-Za-z_][\\w]*).*"), "$1")] -
        cur_offset - 1);
  }
}

class WordIterator {
  static const std::regex word_regex;
  static const std::sregex_iterator null_it;
  std::string s_;
  std::sregex_iterator it_;

 public:
  WordIterator(const std::string &s) : s_(s) {
    it_ = std::sregex_iterator(s_.begin(), s_.end(), word_regex);
  }
  WordIterator &operator++() {
    it_++;
    return *this;
  }
  operator std::string() const {
    if (it_ == null_it) return "";
    return it_->str();
  }
};
const std::sregex_iterator WordIterator::null_it;
const std::regex WordIterator::word_regex(std::string("(\\S+)"));

void Read() {
  static const std::unordered_set<std::string> keywords = {
      "add",  "and",   "not",   "ld",    "ldr",   "ldi",      "lea",
      "st",   "str",   "sti",   "trap",  "getc",  "out",      "puts",
      "in",   "putsp", "halt",  "br",    "brn",   "brz",      "brp",
      "brnz", "brzp",  "brnp",  "brnzp", "jmp",   "ret",      "jsr",
      "jsrr", "rti",   ".orig", ".fill", ".blkw", ".stringz", ".end"};
  Address offset = 0;
  std::string line;
  while (std::cin) {
    getline(std::cin, line);
    auto old_line = line;
    ToLower(line);
    code.push_back(std::make_pair(line, offset));
    WordIterator it(line);
    std::string token = it;
    // std::cout << token << ' ' << offset << std::endl;
    if (token.length() == 0) {
      code.pop_back();
    } else if (!keywords.count(token)) {
      label[token] = offset;
      auto &cur_line = code.rbegin()->first;
      cur_line.replace(cur_line.find(token), token.length(), "");
      token = ++it;
    }
    if (token == ".blkw") {
      offset += std::stoi(
          std::regex_replace(line, std::regex(".*\\.blkw\\D*(\\d+).*"), "$1"));
    } else if (token == ".stringz") {
      offset +=
          std::regex_replace(line, std::regex(".*\"(.*)\".*"), "$1").length() +
          1;  // NOT support "A\"B\""

      WordIterator old_it(old_line);  // restore uppercase
      const std::string old_token = old_it;
      std::string old_token_l = old_token;
      ToLower(old_token_l);
      if (!keywords.count(old_token_l))
        old_line.replace(old_line.find(old_token), token.length(), "");
      code.rbegin()->first = old_line;
    } else if (token == ".end") {
      return;
    } else if (token.length()) {
      offset++;
    }
  }
  throw std::runtime_error(".END not found");
}

void Decode() {
  for (auto &it : code) {
    auto token_it = WordIterator(it.first);
    std::string token = token_it;
    ToLower(token);  // for stringz
    if (token == "add" || token == "and") {
      if (token == "add")
        std::cout << "0001";
      else  // and
        std::cout << "0101";
      auto parsed = GetTwoReg(it.first, token);
      if (parsed.find('r') != std::string::npos) {
        parsed = std::regex_replace(parsed, std::regex(".*r([0-7]).*"), "$1");
        std::cout << "000";
        BinaryPrint<3>(std::stoi(parsed));
      } else {
        std::cout << "1";
        GetImm<5>(parsed);
      }
      std::cout << std::endl;
    } else if (token == "not") {
      std::cout << "1001";
      GetTwoReg(it.first, token);
      std::cout << "111111" << std::endl;
    } else if (token == "ld" || token == "ldi" || token == "lea" ||
               token == "st" || token == "sti") {
      if (token == "ld")
        std::cout << "0010";
      else if (token == "ldi")
        std::cout << "1010";
      else if (token == "lea")
        std::cout << "1110";
      else if (token == "st")
        std::cout << "0011";
      else  // sti
        std::cout << "1011";
      auto parsed = GetOneReg(it.first, token);
      GetOffset<9>(parsed, ",", it.second);
      std::cout << std::endl;
    } else if (token == "ldr" || token == "str") {
      if (token == "ldr")
        std::cout << "0110";
      else  // str
        std::cout << "0111";
      auto parsed = GetTwoReg(it.first, token);
      GetImm<6>(parsed);
      std::cout << std::endl;
    } else if (token == "trap") {
      std::cout << "11110000";
      GetImm<8>(it.first, token);
      std::cout << std::endl;
    } else if (token == "getc") {
      std::cout << "1111000000100000" << std::endl;
    } else if (token == "out") {
      std::cout << "1111000000100001" << std::endl;
    } else if (token == "puts") {
      std::cout << "1111000000100010" << std::endl;
    } else if (token == "in") {
      std::cout << "1111000000100011" << std::endl;
    } else if (token == "putsp") {
      std::cout << "1111000000100100" << std::endl;
    } else if (token == "halt") {
      std::cout << "1111000000100101" << std::endl;
    } else if (token == "br" || token == "brn" || token == "brz" ||
               token == "brp" || token == "brnz" || token == "brzp" ||
               token == "brnp" || token == "brnzp") {
      std::cout << "0000";
      std::cout << ((token == "br" || token.find('n') != std::string::npos)
                        ? '1'
                        : '0');
      std::cout << ((token == "br" || token.find('z') != std::string::npos)
                        ? '1'
                        : '0');
      std::cout << ((token == "br" || token.find('p') != std::string::npos)
                        ? '1'
                        : '0');
      GetOffset<9>(it.first, token, it.second);
      std::cout << std::endl;
    } else if (token == "jmp" || token == "jsrr") {
      if (token == "jmp")
        std::cout << "1100000";
      else  // jsrr
        std::cout << "0100000";
      GetOneReg(it.first, token);
      std::cout << "000000" << std::endl;
    } else if (token == "ret") {
      std::cout << "1100000111000000" << std::endl;
    } else if (token == "jsr") {
      std::cout << "01001";
      GetOffset<11>(it.first, token, it.second);
      std::cout << std::endl;
    } else if (token == "rti") {
      std::cout << "1000000000000000" << std::endl;
    } else if (token == ".orig" || token == ".fill") {
      GetImm<16>(it.first, token);
      std::cout << std::endl;
    } else if (token == ".blkw") {
      auto t = GetImm<16>(it.first, token, false);
      while (t--) std::cout << "0111011101110111" << std::endl;
    } else if (token == ".stringz") {
      auto parsed =
          std::regex_replace(it.first, std::regex(".*\"(.*)\".*"), "$1");
      for (const auto &c : parsed) {
        BinaryPrint<16>(c);
        std::cout << std::endl;
      }
      BinaryPrint<16>(0);
      std::cout << std::endl;
    } else if (token == ".end") {
      return;
    } else {
      throw std::runtime_error("Invalid token: " + std::string(token));
    }
  }
}

int main() {
  Read();
  Decode();
  return 0;
}
