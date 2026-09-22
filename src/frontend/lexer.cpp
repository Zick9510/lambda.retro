// lexer.cpp

#include "Lexer.hpp"

#include "Common.hpp"

Lexer::Lexer() {}

Lexer::Lexer(std::string s) : source(s) {}

// Private

void Lexer::set_source(const std::string& src) {
  cursor = 0;
  source = src;
  tokens = {};

}

bool Lexer::is_end(uint64_t offset) const { /* Bounds-check cursor with source's file */
  return cursor + offset >= source.size();
}

char Lexer::current() const { /* Returns current character in source */
  return is_end() ? '\0' : source[cursor];
}

char Lexer::peek(uint64_t offset) const { /* Returns next character in source */
  return is_end(offset) ? '\0' : source[cursor + offset + 1];
}

char Lexer::get() { /* Consumes next character in source and returns it */
  return is_end() ? '\0' : source[cursor++];
}

bool Lexer::match(char c) { /* Consumes next character if it's equal as the one provided */
  if (is_end() || c != peek()) return false;
  get();
  return true;
}

bool Lexer::_check_number(const std::string& str) {

  char base = '\0';
  size_t i = (size_t)str.data();

  if (str.length() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'b' || str[1] == 'o')) {
    base = str[1];
    i += 2;
  }

  while (i < str.length()) {

    switch (base) {

      case '\0':
        if (('0' > str[i] || str[i] > '9') && str[i] != '_') { return false; }
        break;

      case 'b':
        if (str[i] != '0' && str[i] != '1' && str[i] != '_') { return false; }
        break;

      case 'o':
        if (('0' > str[i] || str[i] > '7') && str[i] != '_') { return false; }
        break;

      case 'x':
        if (!('0' <= str[i] && str[i] <= '9' ||
              'a' <= str[i] && str[i] <= 'f' ||
              'A' <= str[i] && str[i] <= 'F' || str[i] != '_')) { return false; }
        break;

    }

    i++;

  }

  return true;

}

void Lexer::_read_number() {
  uint64_t start = cursor;

  while (

    '9' >= current() && current() >= '0' || current() == '_' ||
    'f' >= current() && current() >= 'a' ||
    'F' >= current() && current() >= 'A' ||

    // current() == 'b' // Redudant.

    current() == 'x' || current() == 'o'

  ) {
    get();
  }

  std::string_view value(source.data() + start, cursor - start);

  _check_number(std::string(value));

  tokens.push_back( {
    TokenKind::NUMBER,
    std::string(value)
  } );

}

void Lexer::_read_word() {
  uint64_t start = cursor;
  TokenKind kind = TokenKind::IDENTIFIER;

  while (

    current() != ' ' && !SYMBOLS.contains(current()) &&
    current() != ',' &&
    current() != '#' &&
    current() != '$' &&
    current() != '"' && current() != '\'' &&

    current() != '\t' &&
    current() != '\n' &&
    current() != '\0'

  ) { get(); }

  std::string value(source.data() + start, cursor - start);

  if (KEYWORDS.count(value)) kind = KEYWORDS.at(value);

  tokens.push_back( {
    kind,
    std::string(value)
  } );

}

void Lexer::_handle_char() {

  get(); // '

  if (current() == '\\') {
    get();
    _read_number();

  } else {

    uint64_t n = get();

    Token t;
    t.kind = TokenKind::NUMBER;
    t.lexeme = std::to_string(n);
    tokens.push_back(t);

   }

  if (current() == '\'') { get(); }

}

// Public

void Lexer::print() {

  for (size_t i = 0; i < tokens.size(); ++i) {

    if (tokens[i].lexeme == "\n") {
      std::cout << "'\\n'";

    } else {
      std::cout << "'" << tokens[i].lexeme << "'";

    }

    std::cout << '\n';

  }

}

std::vector<Token> Lexer::tokenize() {

  while (!is_end()) {

    // Comments
    if (current() == ',') {
      get();

      while (!is_end() && current() != '\n') {
        if (current() == ',') { get(); break; }
        get();

      }

      continue;

    }

    // Structural Symbols \.(){};

    if (SYMBOLS.count(current())) {

      Token t;
      t.kind   = SYMBOLS.at (      current());
      t.lexeme = std::string(1, current());

      tokens.push_back(t);

      get();
      continue;

    }

    // Chars

    if (current() == '\'') {
      _handle_char();
      continue;
    }

    // Everything else

    switch (current()) {

      // Whitespace

      case '\n':
        tokens.push_back( {TokenKind::NEW_LINE, "\n"} );
        get();
        break;

      case '\t':
      case ' ' :
        get();
        break;

      // Numbers

      case '0' ... '9':
        _read_number();
        break;

      // Ids
      case 'a' ... 'z':
      case 'A' ... 'Z':
      case '_':
      default:
        _read_word();
        break;
    }

  }

  tokens.push_back( {TokenKind::END_FILE, ""} );

  return tokens;

}
