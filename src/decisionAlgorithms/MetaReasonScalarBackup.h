#pragma once
#include "../utility/PriorityQueue.h"
#include "DecisionAlgorithm.h"
#include <functional>
#include <memory>

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

    vector<shared_ptr<Node>> backup(
      PriorityQueue<shared_ptr<Node>>& open, shared_ptr<Node> start,
      unordered_map<State, shared_ptr<Node>, Hash>&)
    {
        shared_ptr<Node> goalPrime = open.top();

        // Only move one step towards best on open
        while (goalPrime->getParent() != start)
            goalPrime = goalPrime->getParent();

        return vector<shared_ptr<Node>>(goalPrime);
    }

private:
protected:
    string decisionModule;
};
