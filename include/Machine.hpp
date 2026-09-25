// Machine.hpp

#pragma once

#include "Common.hpp"

class Interpreter {
private:

  MainConfig& config;

  std::unordered_map<std::string, NodeId> env;

  NodeId expand_env(NodeId expr);

public:

  Arena arena;

  Interpreter(MainConfig& c);

  void execute(const Block* program);

  void print() const;

};
