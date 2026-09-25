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
  char c = 'a' + (depth % 26);
  uint32_t suffix = depth / 26;
  return suffix == 0 ? std::string(1, c) : std::string(1, c) + std::to_string(suffix);
}

NodeId unwrap(Arena& arena, NodeId id) {

  while (id != NULL_NODE) {
    const Node& n = arena.get(id);

    if (auto* ind = std::get_if<Indirection>(&n.data)) {
      id = ind->target;

    } else {
      break;

    }

  }

  return id;

}

bool get_church_value(Arena& arena, NodeId node_id, uint64_t& out_val) {
  NodeId curr = unwrap(arena, node_id);
  if (curr == NULL_NODE) return false;

  const Node* n1 = &arena.get(curr);
  auto* f1 = std::get_if<Func>(&n1->data);
  if (!f1) return false;

  NodeId body1 = unwrap(arena, f1->body);
  if (body1 == NULL_NODE) return false;

  const Node* n2 = &arena.get(body1);
  auto* f2 = std::get_if<Func>(&n2->data);
  if (!f2) return false;

  NodeId scan = unwrap(arena, f2->body);
  uint64_t count = 0;

  while (scan != NULL_NODE) {
    const Node& scan_node = arena.get(scan);

    if (auto* app = std::get_if<App>(&scan_node.data)) {

      NodeId fn = unwrap(arena, app->func);
      const Node& fn_node = arena.get(fn);
      auto* var = std::get_if<Var>(&fn_node.data);

      if (!var || var->index != 1) return false;

      count++;
      scan = unwrap(arena, app->arg);

    } else if (auto* var = std::get_if<Var>(&scan_node.data)) {

      if (var->index == 0) {
        out_val = count;
        return true;

      }

      return false;

    } else {
      return false;

    }
  }

  return false;

}

bool decode_pair(Arena& arena, NodeId node_id, NodeId& out_first, NodeId& out_second) {
  NodeId curr = unwrap(arena, node_id);
  if (curr == NULL_NODE) return false;

  const Node& n = arena.get(curr);
  auto* f = std::get_if<Func>(&n.data);
  if (!f) return false;

  NodeId body = unwrap(arena, f->body);
  if (body == NULL_NODE) return false;

  const Node& body_node = arena.get(body);
  auto* outer_app = std::get_if<App>(&body_node.data);
  if (!outer_app) return false;

  NodeId func_node_id = unwrap(arena, outer_app->func);
  const Node& func_node = arena.get(func_node_id);
  auto* inner_app = std::get_if<App>(&func_node.data);
  if (!inner_app) return false;

  NodeId inner_func_id = unwrap(arena, inner_app->func);
  const Node& inner_func_node = arena.get(inner_func_id);
  auto* var = std::get_if<Var>(&inner_func_node.data);
  if (!var || var->index != 0) return false;

  out_first = inner_app->arg;
  out_second = outer_app->arg;

  return true;

}

bool decode_list(Arena& arena, NodeId node_id, std::vector<NodeId>& elements, NodeId& terminator) {
  elements.clear();
  NodeId curr = node_id;

  while (curr != NULL_NODE) {
    NodeId head, tail;

    if (decode_pair(arena, curr, head, tail)) {
      elements.push_back(head);
      curr = tail;

    } else {
      terminator = curr;
      break;

    }
  }

  return !elements.empty();

}

void print_node(Arena& arena, NodeId node_id, uint32_t depth) {
  if (node_id == NULL_NODE) { std::cout << "<null>"; return ; }

  node_id = unwrap(arena, node_id);
  const Node& node = arena.get(node_id);

  uint64_t church_val = 0;

  if (get_church_value(arena, node_id, church_val)) {
    std::cout << color::NUMBER << church_val << color::RESET << ' ';
    return ;
  }

  std::vector<NodeId> list_elements;
  NodeId terminator = NULL_NODE;

  if (decode_list(arena, node_id, list_elements, terminator)) {

    std::vector<std::pair<NodeId, NodeId>> map_pairs;
    bool is_map = true;

    for (NodeId elem : list_elements) {

      NodeId k, v;

      if (decode_pair(arena, elem, k, v)) {
        map_pairs.push_back({k, v});

      } else {
        is_map = false;
        break;

      }

    }

    if (is_map && !map_pairs.empty()) {

      std::cout << color::SYM << "{ " << color::RESET;

      for (size_t i = 0; i < map_pairs.size(); ++i) {
        print_node(arena, map_pairs[i].first, depth);
        std::cout << color::SYM << ": " << color::RESET;
        print_node(arena, map_pairs[i].second, depth);
        std::cout << ", ";
      }

      print_node(arena, terminator, depth);

      std::cout << color::SYM << "} " << color::RESET;
      return ;

    } else {
      std::cout << color::SYM << "[ " << color::RESET;

      for (size_t i = 0; i < list_elements.size(); ++i) {
        print_node(arena, list_elements[i], depth);
      }

      print_node(arena, terminator, depth);

      std::cout << color::SYM << "] " << color::RESET;

      return ;

    }

  }

  if (auto* g = std::get_if<Global>(&node.data)) {
    std::cout << get_color(g->name) << g->name << color::RESET;

  } else if (auto* v = std::get_if<Var>(&node.data)) {
    uint64_t target_depth = depth - 1 - v->index;
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
    print_node(arena, a->arg , depth);
    std::cout << ')';

  } else if (auto* n = std::get_if<Nat>(&node.data)) {
    std::cout << color::NUMBER << n->value << color::RESET;

  } else if (auto* b = std::get_if<Builtin>(&node.data)) {
    std::cout << color::BUILTIN << OPERATORS.at(b->op) << color::RESET;

  }

  std::cout << ' ';

}
