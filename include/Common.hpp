// Common.hpp

#pragma once

#include "Include.hpp"

enum class TokenKind : uint8_t {

  IDENTIFIER, // Identifiers, variables, etc.
  NUMBER    , // Numbers, floats, etc.
  STRING    ,

  LAMBDA    , // \ / lambda

  END_FILE  , //
  NEW_LINE  , // \n

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
  IMPORT,

  // Builtins

  // Arithmetic

  ADD ,
  SUB ,
  MULT,

  // Extra

  FIX,

};

static const std::unordered_map<TokenKind, std::string> KIND_TO_STRING = {
  { TokenKind::IDENTIFIER, "IDENTIFIER" },
  { TokenKind::NUMBER, "NUMBER" },
  { TokenKind::LAMBDA, "LAMBDA" },

  { TokenKind::END_FILE, "END OF FILE" },
  { TokenKind::NEW_LINE, "NEW LINE" },

  { TokenKind::DOT, "." },
  { TokenKind::COLON, "," },
  { TokenKind::SEMICOLON, ";" },

  { TokenKind::LPAREN, "(" },
  { TokenKind::RPAREN, ")" },

  { TokenKind::LCURLY, "{" },
  { TokenKind::RCURLY, "}" },

  { TokenKind::LBRACKET, "[" },
  { TokenKind::RBRACKET, "]" },

  { TokenKind::PIPE, "|" },

  { TokenKind::ERROR, "[ERROR]" },

  { TokenKind::FUNC, "func" },

  { TokenKind::IMPORT, "import" }

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

  static const std::string LAMBDA  = GREEN;
  static const std::string SYM     = RED  ;
  static const std::string NUMBER  = B_RED;
  static const std::string BUILTIN = BLUE ;

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
  {"import", TokenKind::IMPORT},

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

class ASTNode {
public:

  virtual ~ASTNode() = default;
  virtual void print() const = 0;

};

template <typename Base, typename Derived>
class BaseNode : public Base {
public:

  std::unique_ptr<Base> clone() const override {
    return std::make_unique<Derived>(static_cast<const Derived&>(*this));
  }

};

using NodeId = uint32_t;
constexpr NodeId NULL_NODE = 0xffffffff;

struct Global  { std::string name; };
struct Var     { uint32_t index; };
struct App     { NodeId func; NodeId arg; };
struct Func    { NodeId body; };
struct Nat     { uint64_t value; };
struct Builtin { OP op; };

struct EnvNode { NodeId value; NodeId next_env; };
struct Closure { NodeId body ; NodeId      env; };

struct Indirection { NodeId target; };

using NodeVariant = std::variant<Global, Var, App, Func, Nat, Builtin, EnvNode, Closure, Indirection>;

struct Node { NodeVariant data; };

class Arena {
private:
  std::vector<Node> pool;

public:
  Arena() { pool.reserve(0xffff); }

  template <typename T>
  NodeId alloc(T&& data) {
    NodeId id = pool.size();
    pool.push_back(Node{std::forward<T>(data)});
    return id;
  }

  Node& get(NodeId id) { return pool[id]; }

};

std::string _clean_str(const std::string& str);

uint64_t _parse_str(const std::string& str);

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
	NodeId expr;

	StatementExpr(NodeId e)
		: expr(e) {}

	StatementExpr(const StatementExpr& other)
		: expr(other.expr) {}

	void print() const override {
    std::cout << expr << ' ';
	}

};

class StatementFuncDecl : public BaseNode<Statement, StatementFuncDecl> {
public:
  std::string name;

  std::vector<std::string> args;

  NodeId body;

  StatementFuncDecl(

    std::string n,
    std::vector<std::string> a,
    NodeId b

  ) : name(n), args(a), body(b) {}

  StatementFuncDecl(const StatementFuncDecl& other)
    : name(other.name), args(other.args), body(other.body) {}

  void print() const override {}

};

std::string get_name(uint32_t depth);

void print_node(Arena& arena, NodeId node_id, uint32_t depth = 0);

/* --- Config --- */

struct MainConfig {

  std::optional<std::filesystem::path>  input_file;
  std::optional<std::filesystem::path> output_file;

  bool print_env = false;

  bool repl = false;

  uint64_t show_steps = 0;

  bool semicolon_repl_warning = true;

  std::unordered_set<std::string> imported_files{};

};
