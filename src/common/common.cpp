// common.cpp

#include "Common.hpp"

// LambdaVariable

LambdaVariable::LambdaVariable(std::string n) : name(std::move(n)) {}

LambdaVariable::LambdaVariable(const LambdaVariable& other) : name(other.name) {}

void LambdaVariable::print() const {
  std::cout << get_color(name) << name << ' ' << color::RESET;
}

std::unique_ptr<Expression> LambdaVariable::clone() const {
  return std::make_unique<LambdaVariable>(name);
}

std::unique_ptr<Expression> LambdaVariable::substitute(const std::string& var, const Expression* replacement) const {
  if (name == var) return replacement->clone();
  return clone();
}

bool LambdaVariable::contains(const std::string& var) const {
  return name == var;
}

// LambdaApplication

LambdaApplication::LambdaApplication(
  std::unique_ptr<Expression> f,
  std::unique_ptr<Expression> a
) : func(std::move(f)), arg(std::move(a)) {}

LambdaApplication::LambdaApplication(const LambdaApplication& other)
  : func(other.func->clone()), arg(other.arg->clone()) {}

bool LambdaApplication::are_ast_equal(const Expression* a, const Expression* b) {
  if (!a && !b) return true;
  if (!a || !b) return false;

  if (auto va = dynamic_cast<const LambdaVariable*>(a)) {
    auto vb = dynamic_cast<const LambdaVariable*>(b);
    return vb && va->name == vb->name;
  }

  if (auto fa = dynamic_cast<const LambdaFunction*>(a)) {
    auto fb = dynamic_cast<const LambdaFunction*>(b);
    return fb && fa->args == fb->args && are_ast_equal(fa->body.get(), fb->body.get());
  }

  if (auto aa = dynamic_cast<const LambdaApplication*>(a)) {
    auto ab = dynamic_cast<const LambdaApplication*>(b);
    return ab && are_ast_equal(aa->func.get(), ab->func.get()) && are_ast_equal(aa->arg.get(), ab->arg.get());
  }

  return false;

}

bool LambdaApplication::_is_same_combiner(const Expression* a, const Expression* b) {
  if (auto va = dynamic_cast<const LambdaVariable*>(a)) {
    if (auto vb = dynamic_cast<const LambdaVariable*>(b)) {
      return va->name == vb->name;
    }
  }

  if (auto ba = dynamic_cast<const ExpressionBuiltin*>(a)) {
    if (auto bb = dynamic_cast<const ExpressionBuiltin*>(b)) {
      return ba->op == bb->op;
    }
  }

  if (auto fa = dynamic_cast<const LambdaFunction*>(a)) {
    if (auto fb = dynamic_cast<const LambdaFunction*>(b)) {
      return are_ast_equal(a, b);
    }
  }

  return false;

}

struct LambdaApplication::ListData {
  bool is_valid = false;
  const Expression* combiner = nullptr;
  std::vector<const Expression*> elements;
  const Expression* terminator = nullptr;
};

LambdaApplication::ListData LambdaApplication::_extract_list() const {

  ListData data;

  const Expression* current = this;

  while (true) {

    auto app_outer = dynamic_cast<const LambdaApplication*>(current);
    if (!app_outer) { data.terminator = current; break; }

    auto app_inner = dynamic_cast<const LambdaApplication*>(app_outer->func.get());
    if (!app_inner) { data.terminator = current; break; }

    const Expression* current_combiner = app_inner->func.get();

    if (!data.combiner) {
      data.combiner = current_combiner;

    } else if (!_is_same_combiner(data.combiner, current_combiner)) {
      data.terminator = current;
      break;

    }

    data.elements.push_back(app_inner->arg.get());
    current = app_outer->arg.get();

  }

  if (!data.elements.empty()) { data.is_valid = true; }

  return data;

}

bool LambdaApplication::_is_map(const ListData& data) const {
  if (!data.is_valid) return false;

  for (const auto& e : data.elements) {

    auto e_app = dynamic_cast<const LambdaApplication*>(e);
    if (!e_app) return false;

    auto ek_app =dynamic_cast<const LambdaApplication*>(e_app->func.get());
    if (!ek_app) return false;

    if (!_is_same_combiner(data.combiner, ek_app->func.get())) return false;

  }

  return true;

}

void LambdaApplication::_print_as_list(const ListData& data) const {
  std::cout << color::RED << "[ " << color::RESET;
  data.combiner->print();
  std::cout << color::RED << "| " << color::RESET;

  for (const auto& e : data.elements) {
    e->print();
  }

  data.terminator->print();
  std::cout << color::RED << "] " << color::RESET;

}

void LambdaApplication::_print_as_map(const ListData& data) const {
  std::cout << color::RED << "{ " << color::RESET;
  data.combiner->print();
  std::cout << color::RED << "| " << color::RESET;

  for (const auto& e : data.elements) {
    auto e_app  = dynamic_cast<const LambdaApplication*>(e);
    auto ek_app = dynamic_cast<const LambdaApplication*>(e_app->func.get());

    const Expression* key   = ek_app->arg.get();
    const Expression* value = e_app ->arg.get();

    std::cout << "( ";
    key->print();
    std::cout << color::RED << ": " << color::RESET;
    value->print();
    std::cout << ") ";

  }

  data.terminator->print();
  std::cout << color::RED << "} " << color::RESET;

}

void LambdaApplication::print() const {
  ListData data = _extract_list();

  if (data.is_valid) {
    if      (_is_map(data))             { _print_as_map (data); return ; }
    else if (data.elements.size() >= 2) { _print_as_list(data); return ; }

  }

  std::cout << "( ";
  func->print();
  arg ->print();
  std::cout << ") ";

}

std::unique_ptr<Expression> LambdaApplication::clone() const {
  return std::make_unique<LambdaApplication>(func->clone(), arg->clone());
}

std::unique_ptr<Expression> LambdaApplication::substitute(const std::string& var, const Expression* replacement) const {

  if (!contains(var)) return clone();

  return std::make_unique<LambdaApplication>(
    func->substitute(var, replacement),
    arg ->substitute(var, replacement)
  );

}

bool LambdaApplication::contains(const std::string& var) const {
  return func->contains(var) || arg->contains(var);
}
// LambdaFunction

LambdaFunction::LambdaFunction(
  std::vector<std::string>    a,
  std::unique_ptr<Expression> b
) : args(a), body(std::move(b)) {}

LambdaFunction::LambdaFunction(const LambdaFunction& other)
  : args(other.args), body(other.body->clone()) {}

int LambdaFunction::_check_church(const Expression* expr, const std::string& f, const std::string& x) {

  if (auto var = dynamic_cast<const LambdaVariable*>(expr)) {
    if (var->name == x) return 0;
    return -1;
  }

  if (auto app = dynamic_cast<const LambdaApplication*>(expr)) {
    if (auto func = dynamic_cast<const LambdaVariable*>(app->func.get())) {
      if (func->name == f) {
        int inner = _check_church(app->arg.get(), f, x);
        if (inner != -1) return inner + 1;
      }
    }
  }

  return -1;

}

int LambdaFunction::_get_church_value(const std::vector<std::string>& args, const Expression* body) {
  std::string f, x;
  const Expression* target = nullptr;

  if (args.size() == 2) {
    f = args[0];
    x = args[1];
    target = body;

  } else if (args.size() == 1) {

    if (auto inner_lf = dynamic_cast<const LambdaFunction*>(body)) {
      if (inner_lf->args.size() == 1) {
        f = args[0];
        x = inner_lf->args[0];
        target = inner_lf->body.get();

      } else { return -1; }
    } else { return -1; }
  } else { return -1; }

  return _check_church(target, f, x);

}

struct LambdaFunction::ReducedData {
  bool is_valid = false;
  std::string param_f;
  std::vector<const Expression*> elements;
  const Expression* terminator = nullptr;
};

LambdaFunction::ReducedData LambdaFunction::_extract_reduced_list(const LambdaFunction* func) {
  ReducedData data;
  if (func->args.size() != 1) return data;

  std::string f_var = func->args[0];
  const Expression* current = func;

  while (auto current_func = dynamic_cast<const LambdaFunction*>(current)) {
    if (current_func->args.size() != 1) break;

    // Buscamos la estructura: ( (f elem) rest )
    auto outer_app = dynamic_cast<const LambdaApplication*>(current_func->body.get());
    if (!outer_app) break;

    auto inner_app = dynamic_cast<const LambdaApplication*>(outer_app->func.get());
    if (!inner_app) break;

    auto var = dynamic_cast<const LambdaVariable*>(inner_app->func.get());
    if (!var || var->name != f_var) break;

    // Si es el primer nivel, guardamos el nombre de la variable de orden superior
    if (data.elements.empty()) {
      data.param_f = f_var;
    }

    data.elements.push_back(inner_app->arg.get());
    current = outer_app->arg.get();

  }

  if (data.elements.size() >= 2 || (data.elements.size() == 1 && current)) {
      data.is_valid = true;
      data.terminator = current;
  }

  return data;

}

void LambdaFunction::print() const {

  int church_val = _get_church_value(args, body.get());

  if (church_val != -1 && church_val != 0) {
    std::cout << color::NUMBER << church_val << ' ' << color::RESET;
    return ;
  }

  ReducedData data = _extract_reduced_list(this);
  if (data.is_valid) {
    std::cout << color::RED << "[ " << color::RESET;

    std::cout << color::B_GREEN << "λ " << get_color(data.param_f) << data.param_f << ' ' << color::RESET;
    std::cout << color::RED << "| " << color::RESET;

    for (const auto& e : data.elements) {
      e->print();
    }

    if (data.terminator) {
      data.terminator->print();
    }

    std::cout << color::RED << "] " << color::RESET;
    return ;
  }

  std::cout << "(" << color::B_GREEN << " λ " << color::RESET;

  for (const auto& a : args) { std::cout << get_color(a) << a << ' '; }

  std::cout << color::RED << ". " << color::RESET;
  body->print();
  std::cout << ") ";

}

std::unique_ptr<Expression> LambdaFunction::clone() const {
  return std::make_unique<LambdaFunction>(args, body->clone());
}

std::unique_ptr<Expression> LambdaFunction::substitute(const std::string& var, const Expression* replacement) const {

  if (!contains(var)) return clone();

  static uint64_t name_id = 0;

  std::vector<std::string> fresh_args{};

  std::unique_ptr<Expression> renamed_body = body->clone();

  for (const auto& arg : args) {


    if (replacement->contains(arg)) {

      std::string base_name = arg;
      size_t hashtag_pos = base_name.find_last_of('#');

      if (hashtag_pos != std::string::npos) {
        base_name = base_name.substr(0, hashtag_pos);
      }

      std::string fresh_name = base_name + "#" + std::to_string(name_id++);
      fresh_args.push_back(fresh_name);

      auto fresh_var = std::make_unique<LambdaVariable>(fresh_name);
      renamed_body = renamed_body->substitute(arg, fresh_var.get());

    } else {
      fresh_args.push_back(arg);

    }

  }

  auto final_body = renamed_body->substitute(var, replacement);

  return std::make_unique<LambdaFunction>(fresh_args, std::move(final_body));

}

bool LambdaFunction::contains(const std::string& var) const {
  if (std::find(args.begin(), args.end(), var) != args.end()) return false;
  return body->contains(var);
}
