#pragma once
#include "../RealTimeSearch.h"
#include <functional>
#include <memory>
#include <unordered_map>

template<class Domain, class Node, class TopLevelAction>
class LearningAlgorithm
{
    typedef typename Domain::State     State;
    typedef typename Domain::HashState Hash;

public:
    virtual void learn(PriorityQueue<shared_ptr<Node>>              open,
                       unordered_map<State, shared_ptr<Node>, Hash> closed) = 0;
};
