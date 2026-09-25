// machine.cpp

#include "Common.hpp"

#include "Machine.hpp"
#include "Reduc.hpp"

Interpreter::Interpreter(MainConfig& c) : config(c) {

  for (const auto& [b, op] : OPERATORS) {
    env[op] = arena.alloc( Builtin { b } );
  }

};

// private

NodeId Interpreter::expand_env(NodeId expr) {

  const Node& node = arena.get(expr);

  if (auto var = std::get_if<Global>(&node.data)) {

    auto it = env.find(var->name);

    if (it != env.end()) {
      return it->second;
    }

    throw std::runtime_error("Error: Undefined variable '" + var->name + "'\n");

  }

  if (auto func = std::get_if<Func>(&node.data)) {

    NodeId body_id = func->body;

    return arena.alloc( Func { expand_env(body_id) } );
  }

  if (auto app = std::get_if<App>(&node.data)) {

    NodeId func_id = app->func;
    NodeId  arg_id = app-> arg;

    NodeId new_func = expand_env(func_id);
    NodeId new_arg  = expand_env(arg_id );

    return arena.alloc( App { new_func, new_arg } );
  }

  if (std::holds_alternative<Var>    (node.data) ||
      std::holds_alternative<Nat>    (node.data) ||
      std::holds_alternative<Builtin>(node.data)) {
    return expr;
  }

  return NULL_NODE;

}

// public

void Interpreter::execute(const Block* program) {

  for (const auto& stmt : program->instr) {

    if (auto decl = dynamic_cast<const StatementFuncDecl*>(stmt.get())) {

      env[decl->name] = expand_env(decl->body);

      std::cout << color::GREEN << " λ  " << color::RESET << "[ " << get_color(decl->name) << decl->name << color::RESET << " ( ";

      for (const auto& a : decl->args) { std::cout << get_color(a) << a << color::RESET << ' '; }

      std::cout << ") : { ";

      print_ast(arena, decl->body);

      std::cout << "} ]\n\n";

    } else if (auto expr = dynamic_cast<const StatementExpr*>(stmt.get())) {

      std::cout << color::RED << " *  " << color::RESET << "[ ";

      print_ast(arena, expr->expr);

      std::cout << "]\n";

      NodeId current = expand_env(expr->expr);

      uint64_t step = 0;

      BetaReducer reducer;

      while (true) {

        auto [did_step, next_expr] = reducer.step(arena, current);

        if (!did_step) break;

        current = next_expr;

        if (config.show_steps && step % config.show_steps == 0) {
          std::cout << color::YELLOW << " >  " << color::RESET << "[ ";
          print_ast(arena, current);
          std::cout << "]\n";
        }

        step++;

      }

      auto printable = reducer.deepReduce(arena, current);

      std::cout << color::BLUE << " -  " << color::RESET << "[ ";
      print_ast(arena, printable);
      std::cout << "]\n\n";

    }

  }

}

void Interpreter::print() const {

  if (env.empty()) { std::cout << "Envrioment is empty\n"; return ; }

  std::cout << "\n=== Envrioment ===\n";

  for (const auto& [name, expr] : env) {
    std::cout << name << " = ";
    print_ast(const_cast<Arena&>(arena), expr);
    std::cout << '\n';
  }

  std::cout << "==================\n";

}
