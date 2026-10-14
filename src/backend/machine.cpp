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

  if (expr == NULL_NODE) { return NULL_NODE; }
  Node node = arena.get(expr);

  std::vector<NodeId> args;
  NodeId curr = expr;
  std::string macro_name = "";

  while (curr != NULL_NODE) {
    Node curr_node = arena.get(curr);
    if (auto app = std::get_if<App>(&curr_node.data)) {
      args.push_back(app->arg);
      curr = app->func;

    } else if ( auto global = std::get_if<Global>(&curr_node.data)) {
      macro_name = global->name;
      break;

    } else {
      break;

    }

  }

  if (!macro_name.empty() && macros.count(macro_name)) {
    std::reverse(args.begin(), args.end());
    MacroDef mac = macros[macro_name];

    if (args.size() != mac.args.size()) {
      throw std::runtime_error("Macro '" + macro_name + "' expected '" + std::to_string(mac.args.size()) + "' arguments, got '" + std::to_string(args.size()));

    }

    NodeId expanded_body = substitute_macro(mac.body, mac.args, args);
    return expand_env(expanded_body);

  }

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

  if (auto q = std::get_if<Quote>(&node.data)) {
    return arena.alloc( Quote { expand_env(q->inner) } );

  }

  if (auto e = std::get_if<Eval>(&node.data)) {
    return arena.alloc( Eval { expand_env(e->inner) } );

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

      print_node(arena, decl->body);

      std::cout << "} ]\n\n";

    } else if (auto expr = dynamic_cast<const StatementExpr*>(stmt.get())) {

      std::cout << color::RED << " *  " << color::RESET << "[ ";

      print_node(arena, expr->expr);

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
          print_node(arena, current);
          std::cout << "]\n";
        }

        step++;

      }

      auto printable = reducer.deepReduce(arena, current);

      std::cout << color::BLUE << " -  " << color::RESET << "[ ";
      print_node(arena, printable);
      std::cout << "]\n\n";

    } else if (auto macro_decl = dynamic_cast<const StatementMacroDecl*>(stmt.get())) {
      macros[macro_decl->name] = MacroDef { macro_decl->args, macro_decl->body };

      std::cout << color::YELLOW << " M  " << color::RESET << "[ " << get_color(macro_decl->name) << macro_decl->name << color::RESET << " ( ";

      for (const auto& a : macro_decl->args) {
        std::cout << get_color(a) << a << color::RESET << ' ';
      }

      std::cout << ") ]\n\n";

    }

  }

}

NodeId Interpreter::substitute_macro(NodeId node_id, const std::vector<std::string>& param, const std::vector<NodeId>& args) {

  if (node_id == NULL_NODE) { return NULL_NODE; }

  Node node = arena.get(node_id);

  if (auto global = std::get_if<Global>(&node.data)) {

    for (size_t i = 0; i < param.size(); ++i) {
      if (global->name == param[i]) { return args[i]; }
    }

    return node_id;

  }

  if (auto app = std::get_if<App>(&node.data)) {
    NodeId new_f = substitute_macro(app->func, param, args);
    NodeId new_a = substitute_macro(app->arg, param, args);
    return arena.alloc( App { new_f, new_a } );
  }

  if (auto func = std::get_if<Func>(&node.data)) {
    NodeId new_body = substitute_macro(func->body, param, args);
    return arena.alloc( Func { new_body } );
  }

  if (auto q = std::get_if<Quote>(&node.data)) {
    return arena.alloc( Quote { substitute_macro(q->inner, param, args) }  );
  }

  if (auto e = std::get_if<Eval>(&node.data)) {
    return arena.alloc( Eval { substitute_macro(e->inner, param, args) } );
  }

  return node_id;

}

void Interpreter::print() const {

  if (env.empty()) { std::cout << "Envrioment is empty\n"; return ; }

  std::cout << "\n=== Envrioment ===\n";

  for (const auto& [name, expr] : env) {
    std::cout << name << " = ";
    print_node(const_cast<Arena&>(arena), expr);
    std::cout << '\n';
  }

  std::cout << "==================\n";

}
