#pragma once
#include "../utility/PriorityQueue.h"
#include <functional>
#include <memory>
#include <unordered_map>

template<class Domain, class Node>
class DecisionAlgorithm
{
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;

public:
    virtual shared_ptr<Node> backup(
      PriorityQueue<shared_ptr<Node>>& open, shared_ptr<Node> start,
      unordered_map<State, shared_ptr<Node>, Hash>& closed, const string&) = 0;

    virtual ~DecisionAlgorithm() = default;
};
