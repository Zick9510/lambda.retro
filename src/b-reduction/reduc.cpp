// reduc.cpp

#include "Reduc.hpp"

#include "Common.hpp"

NodeId BetaReducer::shift(Arena& arena, NodeId node_id, int inc, uint32_t depth) {

  if (node_id == NULL_NODE) { return NULL_NODE; }

  Node node = arena.get(node_id);

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

  if (auto q = std::get_if<Quote>(&node.data)) {
    NodeId shifted = shift(arena, q->inner, inc, depth);
    if (q->inner == shifted) { return node_id; }
    return arena.alloc( Quote { shifted } );
  }

  if (auto e = std::get_if<Eval>(&node.data)) {
    NodeId shifted = shift(arena, e->inner, inc, depth);
    if (e->inner == shifted) { return node_id; }
    return arena.alloc( Eval { shifted } );
  }

  if (auto ind = std::get_if<Indirection>(&node.data)) {
    NodeId shifted = shift(arena, ind->target, inc, depth);
    if (ind->target == shifted) { return node_id; }
    return arena.alloc( Indirection { shifted } );
  }

  return node_id;

}

NodeId BetaReducer::substitute(Arena& arena, NodeId node_id, NodeId arg_id, uint32_t depth) {

  if (node_id == NULL_NODE) { return NULL_NODE; }

  Node node = arena.get(node_id);

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

  if (auto q = std::get_if<Quote>(&node.data)) {
    NodeId sub = substitute(arena, q->inner, arg_id, depth);
    if (q->inner == sub) { return node_id; }
    return arena.alloc( Quote { sub } );
  }

  if (auto e = std::get_if<Eval>(&node.data)) {
    NodeId sub = substitute(arena, e->inner, arg_id, depth);
    if (e->inner == sub) { return node_id; }
    return arena.alloc( Eval { sub } );
  }

  if (auto ind = std::get_if<Indirection>(&node.data)) {
    NodeId shifted = substitute(arena, ind->target, arg_id, depth);
    if (ind->target == shifted) { return node_id; }
    return arena.alloc( Indirection { shifted } );
  }

  return node_id;

}

BetaReducer::BetaReducer() = default;

NodeId BetaReducer::deepReduce(Arena& arena, NodeId node_id) {

  if (node_id == NULL_NODE) { return NULL_NODE; }

  NodeId current = node_id;

  while (true) {
    auto [did_step, next] = step(arena, current);
    if (!did_step) break;
    current = next;
  }

  Node node = arena.get(current);

  if (auto func = std::get_if<Func>(&node.data)) {
    return arena.alloc( Func { deepReduce(arena, func->body) } );

  } else if (auto app = std::get_if<App>(&node.data)) {

    NodeId func_id = app->func;
    NodeId  arg_id = app->arg ;

    NodeId sub_func = deepReduce(arena, func_id);
    NodeId sub_arg  = deepReduce(arena, arg_id );

    return arena.alloc( App { sub_func, sub_arg } );

  } else if (auto q = std::get_if<Quote>(&node.data)) {
    return node_id;

  } else if (auto q = std::get_if<QuoteCombiner>(&node.data)) {
    return node_id;

  } else if (auto e = std::get_if<Eval>(&node.data)) {
    return arena.alloc( Eval { deepReduce(arena, e->inner) } );

  } else if (auto i = std::get_if<Indirection>(&node.data)) {
    return arena.alloc( Indirection { deepReduce(arena, i->target) } );

  }

  return current;

}

[[gnu::hot]] std::pair<bool, NodeId> BetaReducer::step(Arena& arena, NodeId node_id) {

  if (node_id == NULL_NODE) { return { false, NULL_NODE }; }

  Node node = arena.get(node_id);

  if (auto ind = std::get_if<Indirection>(&node.data)) {
    auto [inner_stepped, next_inner] = step(arena, ind->target);
    if (inner_stepped) {
      return { true, arena.alloc( Indirection { next_inner } ) };
    }
    return { true, ind->target };
  }

  if (auto eval = std::get_if<Eval>(&node.data)) {
    auto [inner_stepped, next_inner] = step(arena, eval->inner);

    if (inner_stepped) {
      return { true, arena.alloc( Eval { next_inner } ) };
    }

    Node inner_node = arena.get(eval->inner);
    if (auto quote = std::get_if<Quote>(&inner_node.data)) {
      return { true, quote->inner };
    }

    return { false, node_id };

  }

  auto app = std::get_if<App>(&node.data);
  if (!app) return { false, node_id };

  NodeId func_id = app->func;
  NodeId arg_id  = app->arg ;

  auto [func_stepped, next_func] = step(arena, func_id);
  if (func_stepped) return { true, arena.alloc( App { next_func, arg_id } ) };

  auto [arg_stepped , next_arg ] = step(arena, arg_id);
  if (arg_stepped ) return { true, arena.alloc( App { func_id, next_arg } ) };

  Node f_node = arena.get(func_id);

  if (auto func = std::get_if<Func>(&f_node.data)) {
    NodeId body_id = func->body;
    NodeId substituted = substitute(arena, body_id, arg_id, 0);
    return { true, substituted };
  }

  if (auto num = std::get_if<Nat>(&f_node.data)) {

    Node arg_node = arena.get(arg_id);
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
    Node inner_f = arena.get(f_app->func);

    if (auto builtin = std::get_if<Builtin>(&inner_f.data)) {

      Node lhs_node = arena.get(f_app->arg);
      Node rhs_node = arena.get(arg_id);

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


  if (auto builtin = std::get_if<Builtin>(&f_node.data)) {

    if (builtin->op == OP::FIX) {

      // FIX f -> f (\x . (FIX f) x)

      NodeId shifted_f = shift(arena, arg_id, 1, 0);

      NodeId fix       = arena.alloc( App { func_id, shifted_f } );

      NodeId x         = arena.alloc( Var { 0 } );

      NodeId fix_x     = arena.alloc( App { fix, x } );

      NodeId delayed   = arena.alloc( Func { fix_x } );

      return { true, arena.alloc( App { arg_id, delayed } ) };

    }
  }

  return { false, node_id };

}
