// Reduc.hpp

#pragma once

#include "Common.hpp"

class BetaReducer {

public:

  BetaReducer();

  static std::unique_ptr<Expression> deepReduce(const Expression* node);
  static std::unique_ptr<Expression> reduceNode(const Expression* node);

};
