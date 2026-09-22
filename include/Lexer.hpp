// Lexer.hpp

#pragma once

#include "Common.hpp"

class Lexer {
private:

  uint64_t cursor = 0; // Points to the current character in source
  std::string source;
  std::vector<Token> tokens;

  bool is_end(uint64_t offset = 0) const;

  char current() const;

  char peek(uint64_t offset = 0) const;

  char get();

  bool match(char c);

  bool _check_number(const std::string& str);
  void _read_number();

  void _read_word();

  void _handle_char();


public:

  Lexer();
  Lexer(std::string s);

  void set_source(const std::string& src);

  void print();

  std::vector<Token> tokenize();

};
