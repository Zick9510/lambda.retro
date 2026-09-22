// driver.cpp

#include "Driver.hpp"

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Reduc.hpp"
#include "Machine.hpp"

#include "Common.hpp"

void Driver::start(MainConfig& config) {

  if (config.input_file.has_value()) { // Standard
    std::filesystem::path file = config.input_file.value();

    auto source = readSourceFile(file);
    if (!source) {
      std::cerr << "Error: Couldn't not open file '" << file << "'\n";
      return ;
    }

    Lexer lexer(source.value());

    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(tokens, config);

    std::unique_ptr<Block> ast = parser.parse();

    Interpreter vm;

    vm.execute(ast.get());

    if (config.print_env)
      vm.print();

  } else { // REPL

    Lexer lexer{};

    Parser parser(config);

    Interpreter vm{};

    std::string source;

    while (true) {

      if (!_read(source)) { break; }

      if (source.empty()) { continue; }

      try {
        lexer.set_source(source);

        std::vector<Token> tokens = lexer.tokenize();

        parser.set_tokens(tokens);

        std::unique_ptr<Block> ast = parser.parse();

        vm.execute(ast.get());

      } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }

    }

  }

}

bool Driver::_read(std::string& source) {

  std::cout << color::B_MAGENTA;

  char* raw_input = readline(">>> \033[0m");

  if (raw_input == nullptr) {
    std::cout << "^D\n";
    return false;
  }

  if (raw_input[0] != '\0') {
    add_history(raw_input);
  }

  source = std::string(raw_input);

  free(raw_input);;

  return true;

}

std::optional<std::string> Driver::readSourceFile(const std::filesystem::path& path) const {

  std::error_code ec;

  // Does it exist?

  if (!std::filesystem::exists(path, ec)) {
    std::cerr << "Error: Path '" << path << "' does not exist\n";
    std::cerr << "(Perhaps maybe the path has a typo?)\n";
    return std::nullopt;
  }

  // Is it a file and not a folder / device?

  if (!std::filesystem::is_regular_file(path, ec)) {
    std::cerr << "Error: Path" << path << "' is not a regular file\n";
    std::cerr << "(Are you sure it is a file and not a folder or device?)\n";
    return std::nullopt;
  }

  // Try open the file

  std::ifstream file(path, std::ios::in | std::ios::binary);

  if (!file.is_open()) {
    std::cerr << "Error: Could not open '" << path << "', I am sorry\n";
    std::cerr << "(Maybe it's the read permissions of the file?)\n";
    return std::nullopt;
  }

  // Reading

  try {
    auto size = std::filesystem::file_size(path);
    if (size == 0) return "";

    std::string buffer{};
    buffer.resize(size);

    if (file.read(&buffer[0], size)) return buffer;

  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Error: I couldn't access the file: '" << e.what() << '\n';
    std::cerr << "I am sorry\n";

  } catch (const std::bad_alloc& e) {
    std::cerr << "Error: The file is too big for the avaiable memory\n";
    std::cerr <<  "I am sorry\n";

  }

  return std::nullopt;

}
