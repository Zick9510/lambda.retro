// parser.cpp

#include "Parser.hpp"

#include "Common.hpp"

// Public

Parser::Parser(MainConfig& c) : config(c) {}

Parser::Parser(std::vector<Token> t, MainConfig& c) : tokens(t), config(c) {}

void Parser::set_tokens(std::vector<Token> t) {
  cursor = 0;
  tokens = t;
}

std::unique_ptr<Block> Parser::parse() {

  Block instr{};

  while (peek().kind != TokenKind::END_FILE) {
    instr.push(_parse_statement());

  }

  return std::make_unique<Block>(instr);

}

// Private

bool Parser::is_end() const { return cursor >= tokens.size(); }

Token Parser::prev() const { return cursor > 0 ? tokens[cursor - 1] : error_token; }

Token Parser::peek() const { return is_end() ? tokens.back() : tokens[cursor]; }

Token Parser::get() {
  if (!is_end()) cursor++;
  return prev();
}

bool Parser::match(TokenKind kind) {
  if (is_end()) return false;
  return peek().kind == kind;
}

Token Parser::check(TokenKind kind) {
  if (match(kind)) return get();

  if (kind == TokenKind::SEMICOLON && config.repl) {

    std::cerr << color::RED << "\n[Automatically placed semicolon]\n\n" << color::RESET;

    return Token{TokenKind::SEMICOLON, ";" };

  }

  throw std::runtime_error("Error: Se esperaba un tipo de token {" + std::to_string((uint8_t)kind) + ") y se encontró '" + peek().lexeme + "'\n");

}

std::unique_ptr<Expression> Parser::_parse_expression() {

  if (match(TokenKind::LAMBDA)) {
    get();

    std::vector<std::string> args = { check(TokenKind::IDENTIFIER).lexeme };

    while (peek().kind == TokenKind::IDENTIFIER) {

      args.push_back(get().lexeme);

    }

    check(TokenKind::DOT);
    auto body = _parse_expression();
    return std::make_unique<LambdaFunction>(args, std::move(body));

  }

  auto expr = _parse_primary();

  while (match(TokenKind::LAMBDA) || match(TokenKind::IDENTIFIER) || match(TokenKind::NUMBER  )||
         match(TokenKind::LPAREN) || match(TokenKind::LCURLY    ) || match(TokenKind::LBRACKET)) {

    auto arg = _parse_primary();
    expr = std::make_unique<LambdaApplication>(std::move(expr), std::move(arg));

  }

  return expr;

}

std::unique_ptr<Expression> Parser::_parse_primary() {

  if (peek().kind == TokenKind::LAMBDA) {
    return _parse_expression();
  }

  if (match(TokenKind::IDENTIFIER)) {
    return std::make_unique<LambdaVariable>(get().lexeme);
  }

  if (match(TokenKind::LPAREN)) {
    get();
    auto expr = _parse_expression();
    check(TokenKind::RPAREN);
    return expr;
  }

  if (match(TokenKind::NUMBER)) {
    return std::make_unique<ExpressionNat>(get().lexeme);
  }

  if (match(TokenKind::LBRACKET)) { // Linked Lists
    get();

    auto combiner = _parse_primary();

    check(TokenKind::PIPE);

    std::vector<std::unique_ptr<Expression>> elements;

    while (peek().kind != TokenKind::RBRACKET && peek().kind != TokenKind::END_FILE) {

      elements.push_back(_parse_primary());

    }

    check(TokenKind::RBRACKET);

    if (elements.empty()) {
      throw std::runtime_error("Error: List requires at list one terminator\n");
    }

    auto expr = std::move(elements.back());
    elements.pop_back();

    for (auto it = elements.rbegin(); it != elements.rend(); ++it) {

      auto apply_combiner = std::make_unique<LambdaApplication>(
        combiner->clone(),
        std::move(*it)
      );

      expr = std::make_unique<LambdaApplication>(
        std::move(apply_combiner),
        std::move(expr)
      );

    }

    return expr;

  }

  if (match(TokenKind::LCURLY)) { // Maps
    get();

    auto combiner = _parse_primary();

    check(TokenKind::PIPE);

    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> pairs;

    std::unique_ptr<Expression> terminator = nullptr;

    while (peek().kind != TokenKind::RCURLY && peek().kind != TokenKind::END_FILE) {

      auto first = _parse_primary();

      if (peek().kind == TokenKind::COLON) {
        get();
        auto second = _parse_primary();
        pairs.push_back({std::move(first), std::move(second)});

      } else {
        terminator = std::move(first);
        break;

      }

    }

    check(TokenKind::RCURLY);

    if (!terminator) {
      throw std::runtime_error("Error: Map requires a terminator\n");
    }

    auto expr = std::move(terminator);

    for (auto it = pairs.rbegin(); it != pairs.rend(); ++it) {

      auto build_key = std::make_unique<LambdaApplication>(
        combiner->clone(),
        std::move(it->first)
      );

      auto inner_pair = std::make_unique<LambdaApplication>(
        std::move(build_key),
        std::move(it->second)
      );

      auto build_link = std::make_unique<LambdaApplication>(
        combiner->clone(),
        std::move(inner_pair)
      );

      expr = std::make_unique<LambdaApplication>(
        std::move(build_link),
        std::move(expr)
      );

    }

    return expr;

  }

  throw std::runtime_error("Error: Expected expression\n");

}

std::unique_ptr<Statement> Parser::_parse_statement_expression() {

  std::unique_ptr<Expression> left = _parse_expression();

  check(TokenKind::SEMICOLON);

  return std::make_unique<StatementExpr>(std::move(left));

}

std::unique_ptr<Statement> Parser::_parse_statement() {

  switch (peek().kind) {

    case TokenKind::FUNC: // Function definition
      return _parse_function_declaration();

    default:
      break;

  }

  return _parse_statement_expression();

}

std::unique_ptr<Statement> Parser::_parse_function_declaration() {

  check(TokenKind::FUNC);

  std::string name = check(TokenKind::IDENTIFIER).lexeme;

  check(TokenKind::LPAREN);

  std::vector<std::string> args{};
  std::unordered_set<std::string> args_cache{};

  std::unique_ptr<Expression> body;

  while (peek().kind != TokenKind::RPAREN && peek().kind != TokenKind::END_FILE) {

    std::string a = check(TokenKind::IDENTIFIER).lexeme;

    if (args_cache.contains(a)) {
      throw std::runtime_error("Error: Duplicated argument '" + a + "'\n");
    }

    args.push_back(a);
    args_cache.insert(a);

  }

  check(TokenKind::RPAREN);
  check(TokenKind::LCURLY);

  body = _parse_expression();

  check(TokenKind::RCURLY);

  return std::make_unique<StatementFuncDecl>(name, args, std::move(body));

}
