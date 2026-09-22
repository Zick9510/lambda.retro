// Machine.hpp

#pragma once

#include "Common.hpp"

class Interpreter {
private:

  MainConfig& config;

  std::unordered_map<std::string, std::unique_ptr<Expression>> env;

  std::unique_ptr<Expression> expand_env(const Expression* expr);

public:

  Interpreter(MainConfig& c);

  void execute(const Block* program);

  void print() const;

};
