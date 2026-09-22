// Common.hpp

#pragma once

#include "Include.hpp"

enum class TokenKind : uint8_t {

  IDENTIFIER, // Identifiers, variables, etc.
  NUMBER    , // Numbers, floats, etc.

  LAMBDA    , // \ / lambda

  END_FILE  , //

  DOT       , // .
  COLON     , // :
  SEMICOLON , // ;

  LPAREN    , // (
  RPAREN    , // )

  LCURLY    , // {
  RCURLY    , // }

  LBRACKET  , // [
  RBRACKET  , // ]

  PIPE      , // |

  ERROR,

  // Keywords
  //LAMBDA // This was previously defined
  FUNC,

  // Builtins

  // Arithmetic

  ADD ,
  SUB ,
  MULT,

  // Extra

  FIX,

};

enum class OP : uint8_t {

  ADD ,
  SUB ,
  MULT,

  FIX,

};

static const std::unordered_map<OP, std::string> OPERATORS = {

  {OP::ADD, "add"},
  {OP::SUB, "sub"},
  {OP::MULT, "mult"},

  {OP::FIX, "fix"},

};

static const std::unordered_set<std::string> BUILTINS = {
  "add", "sub", "mult", "fix"
};


namespace color {

  static const std::string RESET   = "\033[0m" ;
  static const std::string RED     = "\033[31m"; static const std::string B_RED     = "\033[91m";
  static const std::string GREEN   = "\033[32m"; static const std::string B_GREEN   = "\033[92m";
  static const std::string YELLOW  = "\033[33m"; static const std::string B_YELLOW  = "\033[93m";
  static const std::string BLUE    = "\033[34m"; static const std::string B_BLUE    = "\033[94m";
  static const std::string MAGENTA = "\033[35m"; static const std::string B_MAGENTA = "\033[95m";
  static const std::string CYAN    = "\033[36m"; static const std::string B_CYAN    = "\033[96m";

  static const std::string NUMBER  = B_RED;

};

inline std::string get_color(const std::string& name) {

  bool lower  = true;
  bool upper  = true;
  bool number = true;

  if (BUILTINS.count(name)) { return color::B_BLUE; }

  std::string not_symbols = "";

  for (uint16_t i = 0; i <= 255; ++i) {
    if (std::isalnum(i) || i == '_') { not_symbols += i; }
  }

  if (name.find_first_not_of(not_symbols) != std::string::npos) { return color::CYAN; } // Contains a symbol

  for (size_t i = 0; i < name.size(); ++i) {

    if (!std::isupper(name[i]) && name[i] != '_' && !std::isdigit(name[i])) upper = false;

    if (!std::islower(name[i]) && name[i] != '_' && !std::isdigit(name[i])) lower = false;

    if (!std::isdigit(name[i]) && name[i] != '_') number = false;

    if (!upper && !lower) break;

  }

  if      (upper ) { return color::B_YELLOW ; }
  else if (lower ) { return color::B_MAGENTA; }
  else if (number) { return color::NUMBER   ; }
  else             { return color::B_YELLOW ; }

}

static const std::unordered_map<std::string, TokenKind> KEYWORDS = {

  {"func"  , TokenKind::FUNC  },
  {"lambda", TokenKind::LAMBDA},

};

static const std::unordered_map<char, TokenKind> SYMBOLS = {
  {'\\', TokenKind::LAMBDA   },
  {'.',  TokenKind::DOT      },
  {':',  TokenKind::COLON    },
  {';',  TokenKind::SEMICOLON},
  {'(',  TokenKind::LPAREN   },
  {')',  TokenKind::RPAREN   },
  {'{',  TokenKind::LCURLY   },
  {'}',  TokenKind::RCURLY   },
  {'[',  TokenKind::LBRACKET },
  {']',  TokenKind::RBRACKET },
  {'|', TokenKind::PIPE     }

};

struct Token {
  TokenKind kind;
  std::string lexeme;
};

static const Token error_token = Token { TokenKind::ERROR, "error" };

class ExpressionNat;
class ExpressionBuiltin;

class Block;
class StatementExpr;

class StatementFuncDecl;

class LambdaFunction;
class LambdaApplication;
class LambdaVariable;

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  virtual void visit(ExpressionNat* node) = 0;
  virtual void visit(ExpressionBuiltin* node) = 0;

  virtual void visit(Block* node) = 0;
  virtual void visit(StatementExpr* node) = 0;

  virtual void visit(StatementFuncDecl* node) = 0;

  virtual void visit(LambdaFunction* node) = 0;
  virtual void visit(LambdaApplication* node) = 0;
  virtual void visit(LambdaVariable* node) = 0;

};

class ASTNode {
public:

  virtual ~ASTNode() = default;
  virtual void print() const = 0;
  virtual void accept(ASTVisitor* visitor) = 0;

};

template <typename Base, typename Derived>
class BaseNode : public Base {
public:

  void accept(ASTVisitor* visitor) override {
    visitor->visit(static_cast<Derived*>(this));
  }

  std::unique_ptr<Base> clone() const override {
    return std::make_unique<Derived>(static_cast<const Derived&>(*this));
  }

};

class Expression : public ASTNode {
public:
  virtual std::unique_ptr<Expression> clone() const = 0;
  virtual std::unique_ptr<Expression> substitute(const std::string& var, const Expression* replacement) const = 0;
  virtual bool contains(const std::string& var) const { return false; };

};

class ExpressionNat : public BaseNode<Expression, ExpressionNat> {
private:

  static std::string _clean_str(const std::string& str) {

    std::string clean;

    for (const auto& c : str) {
      if (c != '_') { clean += c; }
    }

    return clean;

  }

  static uint64_t _parse_str(const std::string& str) {
    if      (str.find("0x", 0) == 0) { return std::stoll(str.substr(2), nullptr, 16); }
    else if (str.find("0b", 0) == 0) { return std::stoll(str.substr(2), nullptr,  2); }
    else if (str.find("0o", 0) == 0) { return std::stoll(str.substr(2), nullptr,  8); }

    else                                     { return std::stoll(str, nullptr, 10); }

  }

public:
  uint64_t value;

  ExpressionNat(const std::string& v) : value(_parse_str(_clean_str(v))) {}

  ExpressionNat(uint64_t v) : value(v) {}

  ExpressionNat(const ExpressionNat& other) : value(other.value) {}

  void print() const override {
    std::cout << color::NUMBER << value << ' ' << color::RESET;
  }

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<ExpressionNat>(value);
  }

  std::unique_ptr<Expression> substitute(const std::string& var, const Expression* replacement) const override {
    return clone();
  }

};

class ExpressionBuiltin : public BaseNode<Expression, ExpressionBuiltin> {
public:
  OP op;

  ExpressionBuiltin(OP o) : op(o) {}

  ExpressionBuiltin(const ExpressionBuiltin& other) : op(other.op) {}

  void print() const override {
    std::cout << color::BLUE << "<builtin: " << color::B_BLUE << OPERATORS.at(op) << color::BLUE << "> " << color::RESET;

  }

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<ExpressionBuiltin>(op);
  }

  std::unique_ptr<Expression> substitute(const std::string& var, const Expression* replacement) const override {
    return clone();
  }

};

class Statement : public ASTNode {
public:
  virtual std::unique_ptr<Statement> clone() const = 0;

};

class Block : public BaseNode<Statement, Block> {
public:
  std::vector<std::unique_ptr<Statement>> instr;

  Block() = default;

  Block(const Block& other) {
   for (const auto& i : other.instr) {
    instr.push_back(i->clone());
   }
  }

  void push(std::unique_ptr<Statement> instruction) {
    instr.push_back(std::move(instruction));
  }

  void print() const override {

		for (size_t i = 0; i < instr.size(); ++i) {
			instr[i]->print();
		}

  }

};

class StatementExpr : public BaseNode<Statement, StatementExpr> {
public:
	std::unique_ptr<Expression> expr;

	StatementExpr(std::unique_ptr<Expression> e)
		: expr(std::move(e)) {}

	StatementExpr(const StatementExpr& other)
		: expr(other.expr->clone()) {}

	void print() const override {
    expr->print();

	}

};

class StatementFuncDecl : public BaseNode<Statement, StatementFuncDecl> {
public:
  std::string name;

  std::vector<std::string> args;

  std::unique_ptr<Expression> body;

  StatementFuncDecl(

    std::string n,
    std::vector<std::string> a,
    std::unique_ptr<Expression> b

  ) : name(n), args(a), body(std::move(b)) {}

  StatementFuncDecl(const StatementFuncDecl& other)
    : name(other.name), args(other.args), body(other.body->clone()) {}

  void print() const override {}

};

// Lambdas

class LambdaVariable : public BaseNode<Expression, LambdaVariable> {
public:

  std::string name;

  LambdaVariable(std::string n);

  LambdaVariable(const LambdaVariable& other);

  void print() const override;

  std::unique_ptr<Expression> clone() const override;

  std::unique_ptr<Expression> substitute(const std::string& var, const Expression* replacement) const override;

  bool contains(const std::string& var) const override;

};

class LambdaApplication : public BaseNode<Expression, LambdaApplication> {
public:

  std::unique_ptr<Expression> func;
  std::unique_ptr<Expression> arg ;

  LambdaApplication(

    std::unique_ptr<Expression> f, // func
    std::unique_ptr<Expression> a  // arg

  );

  LambdaApplication(const LambdaApplication& other);

private:

  static bool are_ast_equal(const Expression* a, const Expression* b);

  static bool _is_same_combiner(const Expression* a, const Expression* b);

  struct ListData;

  ListData _extract_list() const;

  bool _is_map(const ListData& data) const;

  void _print_as_list(const ListData& data) const;

  void _print_as_map(const ListData& data) const;

public:

  void print() const override;

  std::unique_ptr<Expression> clone() const override;

  std::unique_ptr<Expression> substitute(const std::string& var, const Expression* replacement) const override;

  bool contains(const std::string& var) const override;

};

class LambdaFunction : public BaseNode<Expression, LambdaFunction> {
public:

  std::vector<std::string> args;

  std::unique_ptr<Expression> body;

  LambdaFunction(

    std::vector<std::string>    a, // args
    std::unique_ptr<Expression> b  // body

  );

  LambdaFunction(const LambdaFunction& other);

private:

  static int _check_church(const Expression* expr, const std::string& f, const std::string& x);

  static int _get_church_value(const std::vector<std::string>& args, const Expression* body);

  struct ReducedData;

  static ReducedData _extract_reduced_list(const LambdaFunction* func);

public:

  void print() const override;

  std::unique_ptr<Expression> clone() const override;

  std::unique_ptr<Expression> substitute(const std::string& var, const Expression* replacement) const override;

  bool contains(const std::string& var) const override;

};

/* --- Config --- */

struct MainConfig {

  std::optional<std::filesystem::path>  input_file;
  std::optional<std::filesystem::path> output_file;

  bool print_env = false;

  bool repl = false;

};
