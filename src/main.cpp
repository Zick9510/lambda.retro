// main.cpp

#include "Common.hpp"
#include "Include.hpp"

#include "Driver.hpp"

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

    if (args[i] == "--env") {
      config.print_env = true;

    } else if (args[i].starts_with('-')) {
      std::cerr << "Error: Unknown argument " << args[i] << "\n";
      std::exit(1);

    } else {

      if (config.input_file.has_value()) {
        std::cerr << "Error: Multiple input paths provided\n";
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
