#pragma once
#include "../utility/PriorityQueue.h"
#include <functional>
#include <memory>
#include <stack>
#include <unordered_map>

template<class Domain, class Node>
class DecisionAlgorithm
{
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;

public:
    virtual stack<shared_ptr<Node>> backup(
      const PriorityQueue<shared_ptr<Node>>& open, shared_ptr<Node> start,
      const unordered_map<State, shared_ptr<Node>, Hash>& closed_,
      const bool                                          isForceCommit) = 0;

    virtual ~DecisionAlgorithm() = default;
};
