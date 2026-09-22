// Parser.hpp

#pragma once

#include "Common.hpp"

class Parser {
private:

  uint64_t cursor = 0;
  uint64_t line   = 1;
  std::vector<Token> tokens;

  MainConfig& config;

  bool is_end() const;


  Token prev() const;

  void _skip_new_lines();

  Token peek();

  Token get();

  void sync();

  bool match(TokenKind kind);

	Token check(TokenKind kind);

public:

  Parser(MainConfig& c);

  Parser(std::vector<Token> t, MainConfig& c);

  void set_tokens(std::vector<Token> t);

  std::unique_ptr<Block> parse();

private:

  std::unique_ptr<Statement>  _parse_statement_expression();

  std::unique_ptr<Statement>  _parse_statement();

  std::unique_ptr<Expression> _parse_expression();

  std::unique_ptr<Expression> _parse_primary();

  std::unique_ptr<Statement>  _parse_function_declaration();

};
