// main.cpp

#include "Common.hpp"
#include "Include.hpp"

#include "Driver.hpp"

void print_help(char** argv) {

  std::cout << "Hi! Want to get started?\n\n";

  std::cout << color::B_MAGENTA << "What flags does this program has?\n" << color::RESET;

  std::cout << color::B_CYAN << "  --help / -h" << color::RESET << "        Prints this dialog (I hope it helps :D)\n";
  std::cout << color::B_CYAN << "  --no-W-repl" << color::RESET << "        Disables warning in REPL mode when a semicolon is missing (really annoying!!!)\n";
  std::cout << color::B_CYAN << "  --steps    " << color::RESET << "        Shows you each step the machine took to resolve your expression. Note: you can write --steps=n to only print each n steps (very useful when debbuging!)\n";
  std::cout << color::B_CYAN << "  --print-env" << color::RESET << "        Prints variables stored in the program's internal envrioment (such as functions you define or builtins I made for you)\n";

  std::cout << "\n";

  std::cout << color::B_MAGENTA << "How do I use it?\n" << color::RESET;

  std::cout << "  You can execute it specifing one input file by appending the path to it, like so:\n";
  std::cout << "    " << color::B_GREEN << argv[0] << color::B_YELLOW << " <path/to/your/program>\n\n" << color::RESET;

  std::cout << "  Or you can execute it in REPL mode (interactive terminal) by passing no file. Type the line you want to evaluate (or function to declare) and hit enter. You can exit by pressing Ctrl + C or Ctrl + D\n\n";

  std::cout << color::B_MAGENTA << "How can I read the output?\n" << color::RESET;

  std::cout << "  I think having an example of a typical output might help:\n\n";

  std::cout << color::B_MAGENTA << ">>> " << color::RESET << "func INC ( a ) { add a 1 } "
  << color::GREEN << "\n λ  " << color::RESET << "[ " << color::B_YELLOW << "INC " << color::RESET << "( " << color::B_MAGENTA << "a " << color::RESET << ") : { ( ( "
  << color::B_BLUE << "add " << color::B_MAGENTA << "a " << color::RESET << ") " << color::NUMBER << "1 " << color::RESET << ") } ]"
  << color::B_MAGENTA << "\n>>> " << color::RESET << "INC 2;"
  << color::RED << "\n *  " << color::RESET << "[ ( " << color::B_YELLOW << "INC " << color::NUMBER << "2 " << color::RESET << ") ]"
  << color::YELLOW << "\n >  " << color::RESET << "[ ( ( " << color::B_BLUE << "<builtin: add> " << color::NUMBER << "2 " << color::RESET << ") " << color::NUMBER << "1 " << color::RESET << ") ]"
  << color::YELLOW << "\n >  " << color::RESET << "[ " << color::NUMBER << "3 " << color::RESET << "]"
  << color::BLUE << "\n -  " << color::RESET << "[ " << color::NUMBER << "3 " << color::RESET << "]\n";

  std::cout << "\nThe program uses colors depending on what it sees and how those things look:\n"
  << "Words with caps get" << color::B_YELLOW << " yellow" << color::RESET
  << ", builtins " << color::B_BLUE << "blue" << color::RESET
  << ", numbers " << color::NUMBER << "red " << color::RESET
  << "and words with no caps get " << color::B_MAGENTA << "purple\n\n" << color::RESET;

  std::cout << "The little symbols on the left of the expressions are meant to help you follow the code\n"
  << color::GREEN << "λ " << color::RESET << "means \"Hey, function declaration here, pay attention!!\"\n"
  << color::RED << "* " << color::RESET << "denote what the program read, this is what you wrote, think of it as a question\n"
  << color::YELLOW << "> " << color::RESET << "each arrow pointing to the right are the steps it took to get to the final result\n"
  << color::BLUE << "- " << color::RESET << "then this sign is the program telling \"Hey! I have evaluated your expression, here is your answer!\"\n\n";

  std::cout << "Note: if you define a function that contains a symbol, it will colored a" << color::B_CYAN << " cyan" << color::RESET << " tone. This is meant for you to define operators and spot them more easily\n";

  std::cout << "That being said, I highly recommed following a little style I found really nice:\n";
  std::cout << "  Varaibles written only in lower letters, so they get " << color::B_MAGENTA << "purple\n" << color::RESET;
  std::cout << "  And functions in all caps, so it's " << color::B_YELLOW << "yellow\n" << color::RESET;

  std::cout << "But, if you let me say one more thing... Do as you will and do as you want!! This is just a little advice\n\n";

  std::cout << "Phew. That was a lot... "<< color::B_GREEN << "Good luck! ;D\n\n" << color::RESET;

}

void parse_arguments(int argc, char** argv, MainConfig& config) {

  std::vector<std::string_view> args(argv + 1, argv + argc);

  for (size_t i = 0; i < args.size(); ++i) {

    /*if (args[i] == "-o" || args[i] == "--output") {

      if (i + 1 < args.size()) {
        config.output_file = args[++i];
      } else {
        std::cerr << "Error: -o requires a file path\n";
        std::exit(1);
      }

    } else*/

    if (args[i] == "--print-env") {
      config.print_env = true;

    } else if (args[i].starts_with("--steps")) {
      config.show_steps = 1;

      if (args[i].contains('=')) {
        if (args[i].length() > 8) {
          config.show_steps = std::stoi(
            std::string(
              args[i].substr(args[i].find('=') + 1)
            )
          );
        } else {
          std::cerr << color::RED << "[Error] Could not parse --steps argument\n" << color::RESET;
          std::cerr << color::GREEN << "Example: --steps=10\n" << color::RESET;

        }
      }

    } else if (args[i] == "--W-no-repl") {
      config.semicolon_repl_warning = false;

    } else if (args[i] == "--help" || args[i] == "-h") {
      print_help(argv);
      exit(1);

    } else if (args[i].starts_with('-')) {
      std::cerr << color::RED << "[Error] Unknown argument " << color::RESET << args[i] << "\n";
      std::cerr << color::GREEN << "Try running with --help\n";
      std::exit(1);

    } else {

      if (config.input_file.has_value()) {
        std::cerr << color::RED << "[Error] Multiple input paths provided\n" << color::RESET;
        std::exit(1);
      }

      config.input_file = args[i];

    }

  }

  if (!config.input_file.has_value()) { config.repl = true; }

}

int main(int argc, char* argv[]) {

  MainConfig config{};

  parse_arguments(argc, argv, config);

  Driver driver;
  driver.start(config);

  return 0;

}
