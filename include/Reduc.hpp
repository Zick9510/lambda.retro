// Reduc.hpp

#pragma once

#include "Common.hpp"

class BetaReducer : ASTVisitor {
private:

  bool did_step = false;
  std::unique_ptr<Expression> next = nullptr;

public:

  BetaReducer();

  std::pair<bool, std::unique_ptr<Expression>> step(const Expression* node);

  std::unique_ptr<Expression> deepReduce(const Expression* node);
  static std::unique_ptr<Expression> reduceNode(const Expression* node);

  void visit(const LambdaVariable   * node) override;
  void visit(const LambdaApplication* node) override;
  void visit(const LambdaFunction   * node) override;

  void visit(const ExpressionNat    * node) override;
  void visit(const ExpressionBuiltin* node) override;

  void visit(const Block            * node) override { throw std::runtime_error("Cannot reduce Block"); }
  void visit(const StatementExpr    * node) override { throw std::runtime_error("Cannot reduce Statement"); }
  void visit(const StatementFuncDecl* node) override { throw std::runtime_error("Cannot reduce Function Declaration"); }

};
