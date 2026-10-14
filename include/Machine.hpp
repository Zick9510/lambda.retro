// Machine.hpp

#pragma once

#include "Common.hpp"

class Interpreter {
private:

  MainConfig& config;

  std::unordered_map<std::string, NodeId> env;

  std::unordered_map<std::string, MacroDef> macros;

  NodeId expand_env(NodeId expr);

  NodeId substitute_macro(NodeId node_id, const std::vector<std::string>& param, const std::vector<NodeId>& args);

public:

  Arena arena;

  Interpreter(MainConfig& c);

  void execute(const Block* program);

  void print() const;

};
