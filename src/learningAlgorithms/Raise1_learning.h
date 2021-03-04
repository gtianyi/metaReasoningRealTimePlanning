#pragma once
#include <unordered_map>
#include <functional>
#include <memory>
#include "../utility/PriorityQueue.h"
#include "LearningAlgorithm.h"

using namespace std;

template <class Domain, class Node, class TopLevelAction>
class RaiseOne
        : public LearningAlgorithm<Domain, Node, TopLevelAction> {
    typedef typename Domain::State State;
    typedef typename Domain::Cost Cost;
    typedef typename Domain::HashState Hash;

public:
    RaiseOne(Domain& domain) : domain(domain) {}

    void learn(PriorityQueue<shared_ptr<Node>> open,
            unordered_map<State, shared_ptr<Node>, Hash> closed) {
        // Start by initializing every state in closed to inf h
        for (typename unordered_map<State, shared_ptr<Node>, Hash>::iterator
                        it = closed.begin();
                it != closed.end();
                it++) {
            domain.update_two_distribution(it->first, it->first, 1);
        }
    }

protected:
	Domain & domain;
};
