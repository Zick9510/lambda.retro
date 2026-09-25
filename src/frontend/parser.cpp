// parser.cpp

#include "Parser.hpp"

#include "Lexer.hpp"

#include "Common.hpp"

// Public

Parser::Parser(Arena& a, MainConfig& c) : arena(a), config(c) {}

Parser::Parser(std::vector<Token> t, Arena& a, MainConfig& c) : tokens(t), arena(a), config(c) {}

void Parser::set_tokens(std::vector<Token> t) {
  cursor = 0;
  tokens = t;
}

std::unique_ptr<Block> Parser::parse() {

  Block instr{};

  while (peek().kind != TokenKind::END_FILE) {

    try {
      if (match(TokenKind::IMPORT)) {
        _parse_import(instr);

      } else {
        instr.push(_parse_statement());

      }

    } catch (const std::runtime_error& e) {
      std::cerr << e.what();
      sync();

    }

  }

  return std::make_unique<Block>(instr);

}

// Private

bool Parser::is_end() const { return cursor >= tokens.size(); }

Token Parser::prev() const { return cursor > 0 ? tokens[cursor - 1] : error_token; }

void Parser::_skip_new_lines() {
  while (!is_end() && tokens[cursor].kind == TokenKind::NEW_LINE) { line++; cursor++; }
}

Token Parser::peek() {
  _skip_new_lines();
  return is_end() ? tokens.back() : tokens[cursor];
}

Token Parser::get() {
  _skip_new_lines();
  if (!is_end()) cursor++;
  return prev();
}

bool Parser::match(TokenKind kind) {
  if (is_end()) return false;
  return peek().kind == kind;
}

void Parser::sync() {
  while (!is_end()) {
    if (match(TokenKind::SEMICOLON)) { get(); return ; }
    if (match(TokenKind::FUNC)) { return ;}
    get();
  }
}

Token Parser::check(TokenKind kind) {

  if (match(kind)) return get();

  if (kind == TokenKind::SEMICOLON) {

    if (config.repl) {
      if (config.semicolon_repl_warning)
        std::cerr << color::RED << "\n[Automatically placed semicolon]\n\n" << color::RESET;

    } else {
      std::cerr << color::YELLOW << "[Warning] " << color::RESET << "Expected ';' token. Got '" << peek().lexeme << "' instead\n";
      std::cerr << config.input_file.value() << ":" << line << '\n';

    }

    return Token{TokenKind::SEMICOLON, ";" };

  }

  throw std::runtime_error("Error: Expected '" + KIND_TO_STRING.at(kind) + "' token. Got '" + peek().lexeme + "' instead\n");

}

NodeId Parser::_parse_expression() {

  if (match(TokenKind::LAMBDA)) {
    get();

    std::vector<std::string> args = { check(TokenKind::IDENTIFIER).lexeme };

    while (peek().kind == TokenKind::IDENTIFIER) {
      args.push_back(get().lexeme);
    }

    for (const auto& arg : args) {
      scope_stack.push_back(arg);

    }

    check(TokenKind::DOT);
    NodeId body = _parse_expression();
    NodeId expr = body;

    for (size_t i = 0; i < args.size(); ++i) {
      scope_stack.pop_back();
    }

    for (size_t i = 0; i < args.size(); ++i) {
      expr = arena.alloc( Func { expr } );
    }

    return expr;

  }

  return _parse_pipe();

}

NodeId Parser::_parse_application() {

  NodeId expr = _parse_composition();

  while (match(TokenKind::LAMBDA) || match(TokenKind::IDENTIFIER) || match(TokenKind::NUMBER  ) || match(TokenKind::STRING) ||
         match(TokenKind::LPAREN) || match(TokenKind::LCURLY    ) || match(TokenKind::LBRACKET) ) {

    NodeId arg = _parse_composition();
    expr = arena.alloc( App { expr, arg } );

  }

  return expr;

}

NodeId Parser::_parse_primary() {

  if (match(TokenKind::LAMBDA)) {
    return _parse_expression();

  }

  if (match(TokenKind::IDENTIFIER)) {
    std::string name = get().lexeme;

    for (int32_t i = scope_stack.size() - 1; i >= 0; --i) {
      if (scope_stack[i] == name) {
        uint32_t idx = scope_stack.size() - 1 - i;
        return arena.alloc( Var { idx } );
      }
    }

    return arena.alloc( Global { name } );

  }

  if (match(TokenKind::LPAREN)) {
    get();
    NodeId expr = _parse_expression();
    check(TokenKind::RPAREN);
    return expr;
  }

  if (match(TokenKind::NUMBER)) {
    uint64_t val = _parse_str(_clean_str(get().lexeme));
    return arena.alloc( Nat { val } );
  }

  if (match(TokenKind::LBRACKET)) { // Linked Lists
    get();

    NodeId combiner = _parse_primary();

    check(TokenKind::PIPE);

    std::vector<NodeId> elements;

    while (peek().kind != TokenKind::RBRACKET && peek().kind != TokenKind::END_FILE) {

      if (match(TokenKind::STRING)) {
        std::string str = get().lexeme;
        for (char c : str) {
          elements.push_back(arena.alloc( Nat { (uint64_t)(unsigned char)c } ) );
        }
      } else {
        elements.push_back(_parse_primary());

      }

    }

    check(TokenKind::RBRACKET);

    if (elements.empty()) {
      throw std::runtime_error("Error: List requires a terminator\n");
    }

    NodeId expr = elements.back();
    elements.pop_back();

    for (auto it = elements.rbegin(); it != elements.rend(); ++it) {

      NodeId apply_combiner = arena.alloc( App {combiner, *it} );
      expr = arena.alloc( App {apply_combiner, expr } );

    }

    return expr;

  }

  if (match(TokenKind::LCURLY)) { // Maps
    get();
    NodeId combiner = _parse_primary();
    check(TokenKind::PIPE);

    std::vector<std::pair<NodeId, NodeId>> pairs;

    NodeId terminator = NULL_NODE;

    while (peek().kind != TokenKind::RCURLY && peek().kind != TokenKind::END_FILE) {

      NodeId first = _parse_primary();

      if (peek().kind == TokenKind::COLON) {
        get();
        NodeId second = _parse_primary();
        pairs.push_back({first, second});

      } else {
        terminator = first;
        break;

      }

    }

    check(TokenKind::RCURLY);

    if (!terminator) {
      throw std::runtime_error("Error: Map requires a terminator\n");
    }

    NodeId expr = terminator;

    for (auto it = pairs.rbegin(); it != pairs.rend(); ++it) {

      NodeId build_key  = arena.alloc( App { combiner , it->first  } );
      NodeId inner_pair = arena.alloc( App { build_key, it->second } );
      NodeId build_link = arena.alloc( App { combiner , inner_pair } );

      expr = arena.alloc( App { build_link, expr } );

    }

    return expr;

  }

  throw std::runtime_error("[Error line:" + std::to_string(line) + "] Expected expression\n");

}

std::unique_ptr<Statement> Parser::_parse_statement_expression() {

  NodeId left = _parse_expression();

  check(TokenKind::SEMICOLON);

  return std::make_unique<StatementExpr>(left);

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

  NodeId body;

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

  for (const auto& a : args) {
    scope_stack.push_back(a);
  }

  body = _parse_expression();

  for (size_t i = 0; i < args.size(); ++i) {
    scope_stack.pop_back();
  }

  for (size_t i = 0; i < args.size(); ++i) {
    body = arena.alloc( Func { body } );
  }

  check(TokenKind::RCURLY);

  return std::make_unique<StatementFuncDecl>(name, args, body);

}

void Parser::_parse_import(Block& current_block) {

  std::cout << "362 _parse_import\n";

  check(TokenKind::IMPORT);
  std::string filename = check(TokenKind::STRING).lexeme;
  std::cout << "filename: '" << filename << "'\n";
  check(TokenKind::SEMICOLON);

  if (config.imported_files.count(filename) == 0) {
    config.imported_files.insert(filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
      throw std::runtime_error("Error: Could not open improted file '" + filename + "'\n");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    Lexer _lexer(buffer.str());
    std::vector<Token> _tokens = _lexer.tokenize();

    Parser _parser(_tokens, arena, config);
    std::unique_ptr<Block> _block = _parser.parse();

    for (auto& inst : _block->instr) {
      current_block.push(std::move(inst));
    }

  }

}

NodeId Parser::_parse_pipe() {

  NodeId expr = _parse_application();

  while (match(TokenKind::PIPE)) {
    get();

    NodeId rhs = _parse_application();

    std::vector<NodeId> args;

    NodeId curr = rhs;

    while (curr != NULL_NODE && std::holds_alternative<App>(arena.get(curr).data)) {
      args.push_back(std::get<App>(arena.get(curr).data).arg);
      curr = std::get<App>(arena.get(curr).data).func;

    }

    std::reverse(args.begin(), args.end());

    NodeId head = curr;
    NodeId new_expr = arena.alloc( App { head, expr } );

    for (NodeId arg : args) {
      new_expr = arena.alloc( App { new_expr, arg } );
    }

    expr = new_expr;

  }

  return expr;

}

NodeId Parser::_parse_composition() {

  NodeId left = _parse_primary();

  while (match(TokenKind::DOT)) {
    get();

    NodeId right = _parse_primary();

    NodeId var_x = arena.alloc( Var { 0 } );
    NodeId var_g = arena.alloc( Var { 1 } );
    NodeId var_f = arena.alloc( Var { 2 } );

    NodeId g_x   = arena.alloc( App { var_g, var_x } );
    NodeId f_g_x = arena.alloc( App { var_f, g_x   } );

    NodeId compose = arena.alloc( Func {
      arena.alloc( Func {
        arena.alloc( Func { f_g_x } )
      } )
    } );

    NodeId apply_left = arena.alloc( App { compose, left } );
    left = arena.alloc( App { apply_left, right } );

  }

  return left;

}
