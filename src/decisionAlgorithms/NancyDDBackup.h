#pragma once
#include <functional>
#include <limits>
#include <iostream>
#include <memory>
#include "../utility/PriorityQueue.h"

using namespace std;

template <class Node, class TopLevelAction>
class NancyDDBackup{

public:
    static void backup2TLA(vector<shared_ptr<TopLevelAction>>& tlas) {
        // The nancydd backup is backing up the two belief from the frontier
		// to the TLA and update its minimum path cost.
        for (auto tla : tlas) {
            if (tla->open_TLA.empty()) {
                tla->expectedMinimumPathCost =
                        numeric_limits<double>::infinity();
                continue;
            }

            shared_ptr<Node> best = tla->open_TLA.top();

            tla->setBeliefDD( DiscreteDistributionDD(best->getHstartDistribution(),
                    best->getGValue() - tla->topLevelNode->getGValue()));

            tla->setBeliefDD_ps( DiscreteDistributionDD(best->getHstartDistribution_ps(),
                    best->getGValue() - tla->topLevelNode->getGValue()));

            tla->h_TLA = best->getHValue() +
                    (best->getGValue() - tla->topLevelNode->getGValue());

            tla->expectedMinimumPathCost =
                    tla->getBeliefDD().expectedCost() + tla->topLevelNode->getGValue();

        }
    }
};
