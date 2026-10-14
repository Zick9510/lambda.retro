// Reduc.hpp

#pragma once

#include "Common.hpp"

class BetaReducer {
private:

  bool did_step = false;

public:

  BetaReducer();

  NodeId shift(Arena& arena, NodeId node_id, int amount, uint32_t cutoff = 0);

  std::pair<bool, NodeId> step(Arena& arena, NodeId node);

  NodeId substitute(Arena& arena, NodeId node_id, NodeId arg_id, uint32_t depth);

  NodeId deepReduce(Arena& arena, NodeId node_id);

};
