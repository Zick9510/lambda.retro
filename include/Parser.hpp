// Parser.hpp

#pragma once

#include "Common.hpp"

class Parser {
private:

  uint64_t cursor = 0;
  uint64_t line   = 1;
  std::vector<Token> tokens;

  MainConfig& config;
  Arena& arena;

  std::vector<std::string> scope_stack;

  bool is_end() const;

  Token prev() const;

  void _skip_new_lines();

  Token peek();

  Token get();

  void sync();

  bool match(TokenKind kind);

	Token check(TokenKind kind);

public:

  Parser(Arena& a, MainConfig& c);

  Parser(std::vector<Token> t, Arena& a, MainConfig& c);

  void set_tokens(std::vector<Token> t);

  std::unique_ptr<Block> parse();

private:

  std::unique_ptr<Statement>  _parse_statement_expression();

  std::unique_ptr<Statement>  _parse_statement();

  NodeId _parse_expression();

  NodeId _parse_primary();

  std::unique_ptr<Statement>  _parse_function_declaration();

  void _parse_import(Block& current_block);

  NodeId _parse_pipe();

  NodeId _parse_composition();

  NodeId _parse_application();
};
