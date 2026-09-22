// machine.cpp

#include "Common.hpp"

#include "Machine.hpp"
#include "Reduc.hpp"

Interpreter::Interpreter() {

  for (const auto& [b, _] : OPERATORS) {
    env[OPERATORS.at(b)] = std::make_unique<ExpressionBuiltin>(b);
  }

};

// private

std::unique_ptr<Expression> Interpreter::expand_env(const Expression* expr) {

  if (auto var = dynamic_cast<const LambdaVariable*>(expr)) {
    auto it = env.find(var->name);
    if (it != env.end()) return expand_env(it->second.get());
    return var->clone();
  }

  if (auto func = dynamic_cast<const LambdaFunction*>(expr)) {
    return std::make_unique<LambdaFunction>(func->args, expand_env(func->body.get()));
  }

  if (auto app = dynamic_cast<const LambdaApplication*>(expr)) {
    return std::make_unique<LambdaApplication>(
      expand_env(app->func.get()),
      expand_env(app->arg .get())
    );
  }

  if (auto num = dynamic_cast<const ExpressionNat*>(expr)) {
    return num->clone();
  }

  if (auto builtin = dynamic_cast<const ExpressionBuiltin*>(expr)) {
    return builtin->clone();
  }

  return nullptr;

}

// public

void Interpreter::execute(const Block* program) {

  for (const auto& stmt : program->instr) {

    if (auto decl = dynamic_cast<const StatementFuncDecl*>(stmt.get())) {
      auto lambda = std::make_unique<LambdaFunction>(decl->args, decl->body->clone());
      env[decl->name] = std::move(lambda);

      std::cout << color::GREEN << " λ  " << color::RESET << "[ " << get_color(decl->name) << decl->name << color::RESET << " ( ";

      for (const auto& a : decl->args) { std::cout << get_color(a) << a << color::RESET << ' '; }

      std::cout << ") : { ";

      decl->body->print();

      std::cout << "} ]\n\n";

    } else if (auto expr = dynamic_cast<const StatementExpr*>(stmt.get())) {

      std::cout << color::RED << " *  " << color::RESET << "[ ";

      expr->expr->print();

      std::cout << "]\n";

      auto expanded = expand_env(expr->expr.get());
      auto reduced = BetaReducer::reduceNode(expanded.get());

      std::cout << color::BLUE << " -  " << color::RESET << "[ ";
      BetaReducer::deepReduce(reduced.get())->print();
      std::cout << "]\n\n";

    }

  }

}

void Interpreter::print() const {

  if (env.empty()) { std::cout << "Envrioment is empty\n"; return ; }

  std::cout << "\n=== Envrioment ===\n";

  for (const auto& [name, expr] : env) {
    std::cout << name << " = ";
    expr->print();
    std::cout << '\n';
  }

  std::cout << "==================\n";

}
