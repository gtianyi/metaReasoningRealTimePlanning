#pragma once
#include <functional>
#include <limits>
#include <iostream>
#include <memory>
#include <stack>
#include <cassert>
#include "../utility/PriorityQueue.h"
#include "KBestBackup.h"

using namespace std;

template <class Domain, class Node, class TopLevelAction>
class KBestBackupPersistency
        : public KBestBackup<Domain, Node, TopLevelAction> {
    typedef typename Domain::State State;
    typedef typename Domain::Cost Cost;
    typedef typename Domain::HashState Hash;

public:
    KBestBackupPersistency(Domain& domain, double k, double lookahead)
            : KBestBackup<Domain, Node, TopLevelAction>(domain, k, lookahead) {}

    shared_ptr<Node> backup(PriorityQueue<shared_ptr<Node>>& open,
            vector<shared_ptr<TopLevelAction>>& tlas,
            shared_ptr<Node> start,
            unordered_map<State, shared_ptr<Node>, Hash>& closed) {
        this->kBestDecision(tlas);

        // Take the TLA with the lowest expected minimum path cost
        shared_ptr<TopLevelAction> lowestExpectedPathTLA = tlas[0];
        for (auto tla : tlas) {
            if (tla->expectedMinimumPathCost <
                    lowestExpectedPathTLA->expectedMinimumPathCost)
                lowestExpectedPathTLA = tla;
        }


		auto tlaBest = lowestExpectedPathTLA->open_TLA.top();

		if (!persistPath.empty())
			updatePersistFhat();

		auto newfhat = lowestExpectedPathTLA->expectedMinimumPathCost;

		if (persistPath.empty() || newfhat < persistFhat) {
			// if there is no persist path, go head memoize it
			// if we find a better fhat for root, go head memoize it
			memoizePersistPath(lowestExpectedPathTLA);
		} else if (newfhat == persistFhat &&
				tlaBest->getHHatValue() <= persistTarget->getHHatValue()) {
			// if  fhat ties break ties on h-hat, then prefer not persist
			memoizePersistPath(lowestExpectedPathTLA);
		} else {
			// if we find a worse fhat, but previous target is inside LSS,
			// we then still want to memoize it because the learning
			// will update the previous target.
			auto it = closed.find(persistTarget->getState());
			if (it != closed.end() && !it->second->onOpen()) {
				memoizePersistPath(lowestExpectedPathTLA);
			} else {
				cout << "persist" << endl;
				cout << "previous fhat " << persistFhat << endl;
				cout << "new fhat "
					 << lowestExpectedPathTLA->expectedMinimumPathCost << endl;
			}
		}

		shared_ptr<Node> goalPrime = persistPath.top();
		auto goalPrimeG = persistG.top();

		persistPath.pop();
		persistG.pop();

		// take out that small g at top of the chain
		persistFhat = persistFhat - goalPrimeG;

		prevTargetHHAT = persistTarget->getHHatValue();

		return goalPrime;
		//return lowestExpectedPathTLA->topLevelNode;
    }

private:
    void memoizePersistPath(shared_ptr<TopLevelAction> tla) {
        // clear the persist path
        while (!persistPath.empty()) {
            persistPath.pop();
            persistG.pop();
        }

        assert(persistG.empty());
        assert(persistPath.empty());

        auto cur = tla->open_TLA.top();

        while (cur->getParent() != nullptr) {
            persistPath.push(cur);

            auto curG = cur->getGValue();

            cur = cur->getParent();

            auto parentG = cur->getGValue();
            persistG.push(curG - parentG);
        }

        assert(persistG.size() == persistPath.size());

        persistTarget = tla->open_TLA.top();
        persistFhat = tla->expectedMinimumPathCost;
    }

    void updatePersistFhat() {
        persistTarget->setEpsilonH(this->domain.epsilonHGlobal());
        persistTarget->setEpsilonD(this->domain.epsilonDGlobal());
        persistFhat =
                persistFhat - prevTargetHHAT + persistTarget->getHHatValue();
    }

protected:
    std::stack<shared_ptr<Node>> persistPath;
    std::stack<Cost> persistG;
    shared_ptr<Node> persistTarget;
    Cost persistFhat;
    Cost prevTargetHHAT;
};
