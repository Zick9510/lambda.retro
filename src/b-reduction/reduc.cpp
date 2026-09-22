// reduc.cpp

#include "Reduc.hpp"

#include "Common.hpp"

// Public

BetaReducer::BetaReducer() = default;

std::unique_ptr<Expression> BetaReducer::deepReduce(const Expression* node) {

  std::unique_ptr<Expression> current = node->clone();
  while (true) {
    auto [did_step, next] = step(current.get());
    if (!did_step) break;
    current = std::move(next);
  }

  if (auto func = dynamic_cast<LambdaFunction*>(current.get())) {
    return std::make_unique<LambdaFunction>(
      func->args,
      deepReduce(func->body.get())
    );

  } else if (auto app = dynamic_cast<LambdaApplication*>(current.get())) {
    return std::make_unique<LambdaApplication>(
      deepReduce(app->func.get()),
      deepReduce(app->arg .get())
    );
  }

  return current;

}

[[gnu::hot]] std::pair<bool, std::unique_ptr<Expression>> BetaReducer::step(const Expression* node) {

  if (!node) return {false, nullptr};

  node->accept(this);

  return { did_step, std::move(next) };

}

[[gnu::hot]] void BetaReducer::visit(const LambdaVariable* node) {
  did_step = false;
  next = node->clone();
}

[[gnu::hot]] void BetaReducer::visit(const LambdaFunction* node) {

  //auto [body_stepped, next_body] = step(node->body.get());

  //if (body_stepped) {
  //  did_step = true;
  //  next = std::make_unique<LambdaFunction>(node->args, std::move(next_body));
  //  return ;
  //}

  did_step = false;
  next = node->clone();

}

[[gnu::hot]] void BetaReducer::visit(const LambdaApplication* node) {

  auto [func_stepped, next_func] = step(node->func.get());

  if (func_stepped) {
    did_step = true;
    next = std::make_unique<LambdaApplication>(std::move(next_func), node->arg->clone());
    return ;
  }

  auto [arg_stepped, next_arg] = step(node->arg.get());

  if (arg_stepped) {
    did_step = true;
    next = std::make_unique<LambdaApplication>(node->func->clone(), std::move(next_arg));
    return ;
  }

  const Expression* f = node->func.get();
  const Expression* a = node->arg .get();

  if (auto builtin = dynamic_cast<const ExpressionBuiltin*>(f)) {

    if (builtin->op == OP::FIX) {

      static uint64_t var_id = 0;
      std::string v_var = "$v" + std::to_string(var_id++);

      // (fix a)
      auto fix_a = std::make_unique<LambdaApplication>(
        std::make_unique<ExpressionBuiltin>(OP::FIX),
        a->clone()
      );

      // ((fix a) v)
      auto fix_a_v = std::make_unique<LambdaApplication>(
        std::move(fix_a),
        std::make_unique<LambdaVariable>(v_var)
      );

      // \v . ((fix a) v)
      auto delayed_fix = std::make_unique<LambdaFunction>(
        std::vector<std::string>{v_var},
        std::move(fix_a_v)
      );

      // a \v . ((fix a) v)
      auto result = std::make_unique<LambdaApplication>(
        a->clone(),
        std::move(delayed_fix)
      );

      did_step = true;
      next = std::move(result);
      return ;

    }
  }

  if (auto num = dynamic_cast<const ExpressionNat*>(f)) {

    static uint64_t morph_id = 0;
    std::string f_var = "$f" + std::to_string(morph_id);
    std::string x_var = "$x" + std::to_string(morph_id);
    morph_id++;

    std::unique_ptr<Expression> church_body = std::make_unique<LambdaVariable>(x_var);

    for (int64_t i = 0; i < num->value; ++i) {
      church_body = std::make_unique<LambdaApplication>(
        std::make_unique<LambdaVariable>(f_var),
        std::move(church_body)
      );
    }

    auto inner_func = std::make_unique<LambdaFunction>(std::vector<std::string>{x_var}, std::move(church_body));
    auto church_num = std::make_unique<LambdaFunction>(std::vector<std::string>{f_var}, std::move(inner_func));

    did_step = true;
    next = std::make_unique<LambdaApplication>(std::move(church_num), a->clone());

    return ;

  }

  if (auto inner_app = dynamic_cast<const LambdaApplication*>(f)) {
    if (auto builtin = dynamic_cast<const ExpressionBuiltin*>(inner_app->func.get())) {
      if (auto left_int = dynamic_cast<const ExpressionNat*>(inner_app->arg.get())) {
        if (auto right_int = dynamic_cast<const ExpressionNat*>(a)) {
          switch (builtin->op) {
            case OP::ADD:
              did_step = true;
              next = std::make_unique<ExpressionNat>(left_int->value + right_int->value);
              return ;
            case OP::SUB:
              did_step = true;
              next = std::make_unique<ExpressionNat>(left_int->value - right_int->value);
              return ;
            case OP::MULT:
              did_step = true;
              next= std::make_unique<ExpressionNat>(left_int->value * right_int->value);
              return ;
          }
        }
      }
    }
  }

  if (auto func = dynamic_cast<const LambdaFunction*>(f)) {
    if (func->args.empty()) {
      did_step = true;
      next = func->body->clone();
      return ;
    }

    std::string param = func->args.front();
    auto substituted = func->body->substitute(param, a);

    if (func->args.size() == 1) {
      did_step = true;
      next = std::move(substituted);
      return ;

    } else {
      std::vector<std::string> parameters(func->args.begin() + 1, func->args.end());
      did_step = true;
      next = std::make_unique<LambdaFunction>(parameters, std::move(substituted));
      return ;

    }
  }

  did_step = false;
  next = node->clone();

}

[[gnu::hot]] void BetaReducer::visit(const ExpressionNat* node) {
  did_step = false;
  next = node->clone();

}

[[gnu::hot]] void BetaReducer::visit(const ExpressionBuiltin* node) {
  did_step = false;
  next = node->clone();

}
