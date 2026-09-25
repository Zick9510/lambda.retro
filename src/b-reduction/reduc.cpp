// reduc.cpp

#include "Reduc.hpp"

#include "Common.hpp"

// Private

NodeId BetaReducer::shift(Arena& arena, NodeId node_id, int inc, uint32_t depth) {

  if (node_id == NULL_NODE) { return NULL_NODE; }

  const Node& node = arena.get(node_id);

  if (auto var = std::get_if<Var>(&node.data)) {
    if (var->index >= depth) {
      return arena.alloc( Var { (uint32_t)(var->index + inc) } );
    }
    return node_id;
  }

  if (auto func = std::get_if<Func>(&node.data)) {

    NodeId body_id = func->body;
    NodeId shifted_body = shift(arena, body_id, inc, depth + 1);

    if (body_id == shifted_body) { return node_id; }

    return arena.alloc( Func { shifted_body } );
  }

  if (auto app = std::get_if<App>(&node.data)) {

    NodeId func_id = app->func;
    NodeId  arg_id = app-> arg; // Arena's vector might reach full capactity, reallocate on the heap, and this'd became a dangling pointer, so we pull it to the stack

    NodeId shifted_f = shift(arena, func_id, inc, depth);
    NodeId shifted_a = shift(arena, arg_id , inc, depth);

    if (func_id == shifted_f && arg_id == shifted_a) return node_id;

    return arena.alloc( App { shifted_f, shifted_a } );

  }

  return node_id;

}

NodeId BetaReducer::substitute(Arena& arena, NodeId node_id, NodeId arg_id, uint32_t depth) {

  if (node_id == NULL_NODE) { return NULL_NODE; }

  const Node& node = arena.get(node_id);

  if (auto var = std::get_if<Var>(&node.data)) {
    if (var->index == depth) {
      return shift(arena, arg_id, depth, 0);

    } else if (var->index > depth) {
      return arena.alloc( Var { var->index - 1 } );

    }

    return node_id;

  }

  if (auto func = std::get_if<Func>(&node.data)) {

    NodeId body_id = func->body;
    NodeId sub_body = substitute(arena, body_id, arg_id, depth + 1);

    if (body_id == sub_body) return node_id;

    return arena.alloc( Func { sub_body } );

  }

  if (auto app = std::get_if<App>(&node.data)) {

    NodeId func_id    = app->func;
    NodeId arg_id_app = app->arg ;

    NodeId sub_func = substitute(arena, func_id, arg_id, depth);
    NodeId sub_arg  = substitute(arena, arg_id_app, arg_id, depth);

    if (func_id == sub_func && arg_id_app == sub_arg) return node_id;

    return arena.alloc( App { sub_func, sub_arg } );

  }

  return node_id;

}

// Public

BetaReducer::BetaReducer() = default;

NodeId BetaReducer::deepReduce(Arena& arena, NodeId node_id) {

  if (node_id == NULL_NODE) { return NULL_NODE; }

  NodeId current = node_id;

  while (true) {
    auto [did_step, next] = step(arena, current);
    if (!did_step) break;
    current = next;
  }

  const Node& node = arena.get(current);

  if (auto func = std::get_if<Func>(&node.data)) {
    return arena.alloc( Func { deepReduce(arena, func->body) } );

  } else if (auto app = std::get_if<App>(&node.data)) {

    NodeId func_id = app->func;
    NodeId  arg_id = app->arg ;

    NodeId sub_func = deepReduce(arena, func_id);
    NodeId sub_arg  = deepReduce(arena, arg_id );

    return arena.alloc( App { sub_func, sub_arg } );

  }

  return current;

}

[[gnu::hot]] std::pair<bool, NodeId> BetaReducer::step(Arena& arena, NodeId node_id) {

  if (node_id == NULL_NODE) { return { false, NULL_NODE }; }

  const Node& node = arena.get(node_id);

  auto app = std::get_if<App>(&node.data);
  if (!app) return { false, node_id };

  NodeId func_id = app->func;
  NodeId arg_id  = app->arg ;

  auto [func_stepped, next_func] = step(arena, func_id);
  if (func_stepped) return { true, arena.alloc( App { next_func, arg_id } ) };

  auto [arg_stepped , next_arg ] = step(arena, arg_id);
  if (arg_stepped ) return { true, arena.alloc( App { func_id, next_arg } ) };

  const Node& f_node = arena.get(func_id);

  if (auto func = std::get_if<Func>(&f_node.data)) {
    NodeId body_id = func->body;
    NodeId substituted = substitute(arena, body_id, arg_id, 0);
    return { true, substituted };
  }

  if (auto num = std::get_if<Nat>(&f_node.data)) {

    const Node& arg_node = arena.get(arg_id);
    if (auto arg_num = std::get_if<Nat>(&arg_node.data)) { // (m n) = n ** m;
      uint64_t exponent =     num->value;
      uint64_t base     = arg_num->value;

      uint64_t result   = 1;

      for (uint64_t i = 0; i < exponent; ++i) { result *= base; }

      return { true, arena.alloc( Nat { result } ) };

    }

    uint64_t loop_count = num->value; // (n f) = f ( f ( f (...)  ) ); n times

    NodeId shifted_f = shift(arena, arg_id, 1, 0);
    NodeId body = arena.alloc( Var { 0 } );

    for (uint64_t i = 0; i < loop_count; ++i) {
      body = arena.alloc( App { shifted_f, body } );
    }

    return { true, arena.alloc( Func { body } ) };

  }

  if (auto f_app = std::get_if<App>(&f_node.data)) {
    const Node& inner_f = arena.get(f_app->func);

    if (auto builtin = std::get_if<Builtin>(&inner_f.data)) {

      const Node& lhs_node = arena.get(f_app->arg);
      const Node& rhs_node = arena.get(arg_id);

      auto lhs = std::get_if<Nat>(&lhs_node.data);
      auto rhs = std::get_if<Nat>(&rhs_node.data);

      if (lhs && rhs) {
        uint64_t l   = lhs->value;
        uint64_t r   = rhs->value;
        uint64_t res = 0;

        switch (builtin->op) {
          case OP::ADD: res = l + r; break;
          case OP::SUB: res = l - r; break;
          case OP::MULT: res = l * r; break;
          default: throw std::runtime_error("Unrecognized Builtin operator");

        }

        return { true, arena.alloc( Nat { res } ) };

      }
    }
  }

  return { false, node_id };

}
