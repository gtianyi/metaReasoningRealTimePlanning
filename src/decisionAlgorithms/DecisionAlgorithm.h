#pragma once
#include "../utility/PriorityQueue.h"
#include <functional>
#include <memory>
#include <unordered_map>

template<class Domain, class Node, class TopLevelAction>
class DecisionAlgorithm
{
    typedef typename Domain::State     State;
    typedef typename Domain::HashState Hash;

public:
    virtual shared_ptr<Node> backup(
      PriorityQueue<shared_ptr<Node>>&    open,
      vector<shared_ptr<TopLevelAction>>& tlas, shared_ptr<Node> start,
      unordered_map<State, shared_ptr<Node>, Hash>& closed) = 0;

    virtual ~DecisionAlgorithm() = default;
};
