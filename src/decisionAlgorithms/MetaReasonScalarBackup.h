#pragma once
#include "../utility/PriorityQueue.h"
#include <functional>
#include <memory>
#include <stack>

using namespace std;

template<class Domain, class Node>
// class ScalarBackup : public DecisionAlgorithm<Domain, Node, TopLevelAction>
class MetaReasonScalarBackup
{
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;

public:
    MetaReasonScalarBackup(string decisionModule_)
        : decisionModule(decisionModule_)
    {}

    stack<shared_ptr<Node>> backup(
      PriorityQueue<shared_ptr<Node>>& open, shared_ptr<Node> start,
      unordered_map<State, shared_ptr<Node>, Hash>&, const string&)
    {
        stack<shared_ptr<Node>> commitedNodes;
        shared_ptr<Node>         goalPrime = open.top();

        // Only move one step towards best on open
        while (goalPrime->getParent() != start) {
            if (decisionModule == "alltheway") {
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
