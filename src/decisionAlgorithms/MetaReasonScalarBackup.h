#pragma once
#include "../utility/PriorityQueue.h"
#include "DecisionAlgorithm.h"
#include <functional>
#include <memory>

using namespace std;

template<class Domain, class Node>
// class ScalarBackup
class MetaReasonScalarBackup : public DecisionAlgorithm<Domain, Node>
{
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;

public:
    MetaReasonScalarBackup(string decisionModule_)
        : decisionModule(decisionModule_)
    {}

    stack<shared_ptr<Node>> backup(
      const PriorityQueue<shared_ptr<Node>>& open, shared_ptr<Node> start,
      const unordered_map<State, shared_ptr<Node>, Hash>&, const bool)
    {
        stack<shared_ptr<Node>> commitedNodes;
        shared_ptr<Node>        goalPrime = open.top();

        // Only move one step towards best on open
        while (goalPrime->getParent() != start) {
            if (decisionModule == "alltheway" ||
                decisionModule == "dynamicLookahead") {
                commitedNodes.push(goalPrime);
            }

            goalPrime = goalPrime->getParent();
        }

        commitedNodes.push(goalPrime);

        return commitedNodes;
    }

protected:
    string decisionModule;
};
