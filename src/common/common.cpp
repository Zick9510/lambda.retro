// common.cpp

#include "Common.hpp"

std::string _clean_str(const std::string& str) {

    std::string clean;

    for (const auto& c : str) {
      if (c != '_') { clean += c; }
    }

    return clean;

  }

uint64_t _parse_str(const std::string& str) {
  if      (str.find("0x", 0) == 0) { return std::stoll(str.substr(2), nullptr, 16); }
  else if (str.find("0b", 0) == 0) { return std::stoll(str.substr(2), nullptr,  2); }
  else if (str.find("0o", 0) == 0) { return std::stoll(str.substr(2), nullptr,  8); }

  else                                     { return std::stoll(str, nullptr, 10); }

}

std::string get_name(uint32_t depth) {
  char c = 'c' + (depth % 26);
  uint32_t suffix = depth / 26;
  return suffix == 0 ? std::string(1, c) : std::string(1, c) + std::to_string(suffix);
}

void print_node(Arena& arena, NodeId node_id, uint32_t depth) {
  if (node_id == NULL_NODE) { std::cout << "<null>"; return ; }

  const Node& node = arena.get(node_id);

  if (std::holds_alternative<Indirection>(node.data)) {
    print_node(arena, std::get<Indirection>(node.data).target, depth);
    return ;
  }

  if (auto* g = std::get_if<Global>(&node.data)) {
    std::cout << get_color(g->name) << g->name << color::RESET;

  } else if (auto* v = std::get_if<Var>(&node.data)) {
    uint32_t target_depth = depth - 1 - v->index;
    std::string name = get_name(target_depth);
    std::cout << get_color(name) << name << color::RESET;

  } else if (auto* f = std::get_if<Func>(&node.data)) {
    std::string param_name = get_name(depth);
    std::cout << '(' << color::LAMBDA << " λ " << color::RESET << param_name << color::SYM << " . " << color::RESET;
    print_node(arena, f->body, depth + 1);
    std::cout << ')';

  } else if (auto* a = std::get_if<App>(&node.data)) {
    std::cout << "( ";
    print_node(arena, a->func, depth);
    std::cout << ' ';
    print_node(arena, a->arg , depth);
    std::cout << ')';

  } else if (auto* n = std::get_if<Nat>(&node.data)) {
    std::cout << color::NUMBER << n->value << color::RESET;

  } else if (auto* b = std::get_if<Builtin>(&node.data)) {
    std::cout << color::BUILTIN << OPERATORS.at(b->op) << color::RESET;

  }

  std::cout << ' ';

}
