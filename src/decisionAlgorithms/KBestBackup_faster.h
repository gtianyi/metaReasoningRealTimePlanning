#pragma once
#include <functional>
#include <limits>
#include <iostream>
#include <memory>
#include "../utility/PriorityQueue.h"
#include "DecisionAlgorithm.h"

using namespace std;

template <class Domain, class Node, class TopLevelAction>
class KBestBackup : public DecisionAlgorithm<Domain, Node, TopLevelAction> {
    typedef typename Domain::State State;
    typedef typename Domain::Cost Cost;
    typedef typename Domain::HashState Hash;

public:
    KBestBackup(Domain& domain, double k, double lookahead)
            : domain(domain),
              k(k),
              lookahead(lookahead) {}

    virtual shared_ptr<Node> backup(PriorityQueue<shared_ptr<Node>>& open,
            vector<shared_ptr<TopLevelAction>>& tlas,
            shared_ptr<Node> start,
            unordered_map<State, shared_ptr<Node>, Hash>& closed) {
        kBestDecision(tlas);

        // Take the TLA with the lowest expected minimum path cost
        shared_ptr<TopLevelAction> lowestExpectedPathTLA = tlas[0];
        for (auto tla : tlas) {
            if (tla->expectedMinimumPathCost <
                    lowestExpectedPathTLA->expectedMinimumPathCost)
                lowestExpectedPathTLA = tla;
        }

        shared_ptr<Node> goalPrime = lowestExpectedPathTLA->topLevelNode;

        return goalPrime;
    }

protected:

    void kBestDecision(vector<shared_ptr<TopLevelAction>>& tlas) {
        // The K-Best decision assumes that the only nodes within the subtrees
        // of the TLAs are the k-best frontier nodes
        // on their opened lists. Find them.
        for (auto tla : tlas) {
            /*tla->kBestNodes.clear();*/

            //shared_ptr<Node> best = tla->open_TLA.top();
            //tla->open_TLA.pop();

            //// Make this node's PDF a discrete distribution...
            //best->distribution = DiscreteDistribution(100,
                    //best->getFValue(),
                    //best->getFHatValue(),
                    //best->getDValue(),
                    //best->getFHatValue() - best->getFValue());

            //tla->kBestNodes.push_back(best);
            tla->expectedMinimumPathCost =
                    tla->open_TLA.top()->getFHatValue();
        }
    }

protected:
    Domain& domain;
    double k;
    double lookahead;
};
