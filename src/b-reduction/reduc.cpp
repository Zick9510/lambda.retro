// reduc.cpp

#include "Reduc.hpp"

#include "Common.hpp"

// Public

BetaReducer::BetaReducer() = default;

std::unique_ptr<Expression> BetaReducer::deepReduce(const Expression* node) {

  auto reduced = reduceNode(node);

  if (auto func = dynamic_cast<LambdaFunction*>(reduced.get())) {
    return std::make_unique<LambdaFunction>(
      func->args,
      deepReduce(func->body.get())
    );

  } else if (auto app = dynamic_cast<LambdaApplication*>(reduced.get())) {
    return std::make_unique<LambdaApplication>(
      deepReduce(app->func.get()),
      deepReduce(app->arg .get())
    );
  }

  return reduced;

}

std::unique_ptr<Expression> BetaReducer::reduceNode(const Expression* node) {

  // Is it a Variable?
  if (auto var_node = dynamic_cast<const LambdaVariable*>(node)) {
    return var_node->clone();
  }

  // Is it an Integer?
  if (auto num_node = dynamic_cast<const ExpressionNat*>(node)) {
    return num_node->clone();
  }

  // Maybe it's a Builtin?
  if (auto builtin_node = dynamic_cast<const ExpressionBuiltin*>(node)) {
    return builtin_node->clone();
  }

  // Nope. What about a Function?
  if (auto func_node = dynamic_cast<const LambdaFunction*>(node)) {
    return func_node->clone();
  }

  // An Application...?
  if (auto app_node = dynamic_cast<const LambdaApplication*>(node)) {

    auto f = reduceNode(app_node->func.get());
    auto a = reduceNode(app_node->arg.get());

    // Is the first element a builtin?

    if (auto builtin = dynamic_cast<ExpressionBuiltin*>(f.get())) {

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

        // a (\v . ((fix a) v))
        auto result = std::make_unique<LambdaApplication>(
          a->clone(),
          std::move(delayed_fix)
        );

        return reduceNode(result.get());

      }

    }

    // Maybe a number?

    if (auto num = dynamic_cast<ExpressionNat*>(f.get())) {
      if (num->value >= 0) {

        // Name collision proof
        static uint64_t morph_id = 0;
        std::string f_var = "$f" + std::to_string(morph_id);
        std::string x_var = "$x" + std::to_string(morph_id);
        morph_id++;

        std::unique_ptr<Expression> church_body = std::make_unique<LambdaVariable>(x_var);

        for (int i = 0; i < num->value; ++i) {
          church_body = std::make_unique<LambdaApplication>(
            std::make_unique<LambdaVariable>(f_var),
            std::move(church_body)
          );
        }

        auto inner_func = std::make_unique<LambdaFunction>(std::vector<std::string>{x_var}, std::move(church_body));
        auto church_num = std::make_unique<LambdaFunction>(std::vector<std::string>{f_var}, std::move(inner_func));
        auto morphed_app = std::make_unique<LambdaApplication>(std::move(church_num), std::move(a));

        return reduceNode(morphed_app.get());

      }

    }

    // Okay, it might be a builtin (for numbers) inside stuff

    if (auto inner_app = dynamic_cast<LambdaApplication*>(f.get())) {
      if (auto builtin = dynamic_cast<ExpressionBuiltin*>(inner_app->func.get())) {
        if (auto left_int = dynamic_cast<ExpressionNat*>(inner_app->arg.get())) {
          if (auto right_int = dynamic_cast<ExpressionNat*>(a.get())) {

            switch (builtin->op) {

              case OP::ADD:
                return std::make_unique<ExpressionNat>(left_int->value + right_int->value);
              case OP::SUB:
                return std::make_unique<ExpressionNat>(left_int->value - right_int->value);
              case OP::MULT:
                return std::make_unique<ExpressionNat>(left_int->value * right_int->value);

            }
          }
        }
      }
    }

    // Maybe a function?

    if (auto func = dynamic_cast<LambdaFunction*>(f.get())) {

      if (func->args.empty()) return reduceNode(func->body.get()); // No arguments

      std::string param = func->args.front();

      auto substituted = func->body->substitute(param, a.get());

      if (func->args.size() == 1) {
        return reduceNode(substituted.get());

      } else {

        std::vector<std::string> parameters(func->args.begin() + 1, func->args.end());

        auto partial_func = std::make_unique<LambdaFunction>(parameters, std::move(substituted));

        return reduceNode(partial_func.get());

      }

    }

    return std::make_unique<LambdaApplication>(std::move(f), std::move(a));

  }

  // No idea what it was

  return nullptr;

}
