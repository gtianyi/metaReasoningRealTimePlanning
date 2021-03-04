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
    void csernaBackup(shared_ptr<TopLevelAction> tla) {
        // We assume in k-best that only the k-best nodes matter.
        if (!tla->kBestNodes.empty()) {
            // Perform Cserna Backups on k-best nodes
            while (tla->kBestNodes.size() > 1) {
                // Take the first two and do a Cserna backup...
                tla->kBestNodes[0]->distribution =
                        tla->kBestNodes[0]->distribution *
                        tla->kBestNodes[1]->distribution;
                // Remove the other used in the backup
                tla->kBestNodes.erase(remove(tla->kBestNodes.begin(),
                                             tla->kBestNodes.end(),
                                             tla->kBestNodes[1]),
                        tla->kBestNodes.end());
            }

            // Get the expected value of the resulting Cserna Distribution
            tla->expectedMinimumPathCost =
                    tla->kBestNodes[0]->distribution.expectedCost();
            tla->setBelief(tla->kBestNodes[0]->distribution);
        } else {
            // If nothing was expanded under this TLA, use the expected value of
            // the TLA
            tla->expectedMinimumPathCost = numeric_limits<double>::infinity();
        }
    }

    void kBestDecision(vector<shared_ptr<TopLevelAction>>& tlas) {
        // The K-Best decision assumes that the only nodes within the subtrees
        // of the TLAs are the k-best frontier nodes
        // on their opened lists. Find them.
        for (auto tla : tlas) {
            tla->kBestNodes.clear();

            // If this TLA has unique, probably optimal subtrees beneath it, it
            // is valid

            int i = 0;
            // Add to the best k nodes while i < k and non-selected nodes exist
            // on the frontier
            while (i < k && !tla->open_TLA.empty()) {
                shared_ptr<Node> best = tla->open_TLA.top();
                tla->open_TLA.pop();

                // Make this node's PDF a discrete distribution...
                best->distribution = DiscreteDistribution(100,
                        best->getFValue(),
                        best->getFHatValue(),
                        best->getDValue(),
                        best->getFHatValue() - best->getFValue());

                tla->kBestNodes.push_back(best);
                i++;
            }

            // Now that k-best are selected, perform Cserna backup
            csernaBackup(tla);
        }
    }

protected:
    Domain& domain;
    double k;
    double lookahead;
};
