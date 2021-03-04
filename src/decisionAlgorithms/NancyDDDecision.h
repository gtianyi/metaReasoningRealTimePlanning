#pragma once
#include <functional>
#include <limits>
#include <iostream>
#include <memory>
#include <stack>
#include "../utility/PriorityQueue.h"
#include "DecisionAlgorithm.h"
#include "NancyDDBackup.h"

using namespace std;

template <class Domain, class Node, class TopLevelAction>
class NancyDDDecision : public DecisionAlgorithm<Domain, Node, TopLevelAction> {
    typedef typename Domain::State State;
    typedef typename Domain::Cost Cost;
    typedef typename Domain::HashState Hash;

public:
    NancyDDDecision(Domain& domain, double lookahead)
            : domain(domain),
              lookahead(lookahead) {}

    shared_ptr<Node> backup(PriorityQueue<shared_ptr<Node>>& open,
            vector<shared_ptr<TopLevelAction>>& tlas,
            shared_ptr<Node> start,
            unordered_map<State, shared_ptr<Node>, Hash>& closed) {
        // we have to do one more back up, because we havn't back up the frontier
		// after the last expansion.
        NancyDDBackup<Node, TopLevelAction>::backup2TLA(tlas);

        // Take the TLA with the lowest expected minimum path cost
        auto lowestExpectedPathTLA = tlas[0];
        for (auto tla : tlas) {
            if (tla->expectedMinimumPathCost <
                    lowestExpectedPathTLA->expectedMinimumPathCost)
                lowestExpectedPathTLA = tla;
        }

		return lowestExpectedPathTLA->topLevelNode;

   /*     shared_ptr<Node> goalPrime;*/
		//auto tlaBest = lowestExpectedPathTLA->open_TLA.top();

		//if (persistPath.empty() ||
				//lowestExpectedPathTLA->expectedMinimumPathCost < persistFhat) {
			//// if there is no persist path, go head memoize it
			//// if we find a better fhat for root, go head memoize it
			//memoizePersistPath(lowestExpectedPathTLA);
		//} else if (lowestExpectedPathTLA->expectedMinimumPathCost ==
						//persistFhat &&
				//tlaBest->getHHatValueFromDist() <=
						//persistTarget->getHHatValueFromDist()) {
			//// if  fhat ties break ties on h-hat then prefer not persist
			//memoizePersistPath(lowestExpectedPathTLA);
		//} else{
			//// if we find a worse fhat, but previous target is inside LSS, 
			//// we then still want to memoize it because the learning then 
			//// will update the previous target.
			//auto it = closed.find(persistTarget->getState());
			//if (it != closed.end() && !it->second->onOpen()) {
				//memoizePersistPath(lowestExpectedPathTLA);
			//} else {
				//cout << "persist" << endl;
				//cout << "previous fhat " << persistFhat << endl;
				//cout << "new fhat "
					 //<< lowestExpectedPathTLA->expectedMinimumPathCost +
								//lowestExpectedPathTLA->topLevelNode->getGValue()
					 //<< endl;
			//}
		//}

		//goalPrime = persistPath.top();
		//auto goalPrimeG = persistG.top();
		//persistPath.pop();
		//persistG.pop();

		//persistFhat -= goalPrimeG;

		/*return goalPrime;*/
    }

private:
    void memoizePersistPath(shared_ptr<TopLevelAction> tla) {
		//clear the persist path
        while (!persistPath.empty()) {
            persistPath.pop();
			persistG.pop();
        }

        auto cur = tla->open_TLA.top();

        while (cur->getParent() != nullptr) {
            persistPath.push(cur);
			auto curG = cur->getGValue();
            cur = cur->getParent();

			auto parentG = cur->getGValue();
			persistG.push(curG-parentG);
        }

        assert(persistG.size() == persistPath.size());

        persistTarget = tla->open_TLA.top();
        persistFhat = tla->expectedMinimumPathCost;
    }

protected:
    Domain& domain;
    std::stack<shared_ptr<Node>> persistPath;
    shared_ptr<Node> persistTarget;
    std::stack<Cost> persistG;
    Cost persistFhat;
    double lookahead;
};
