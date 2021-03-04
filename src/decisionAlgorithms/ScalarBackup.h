#pragma once
#include <functional>
#include <memory>
#include "../utility/PriorityQueue.h"
#include "DecisionAlgorithm.h"

using namespace std;

template <class Domain, class Node, class TopLevelAction>
class ScalarBackup : public DecisionAlgorithm<Domain, Node, TopLevelAction>
{
	typedef typename Domain::State State;
	typedef typename Domain::Cost Cost;
    typedef typename Domain::HashState Hash;

public:
	ScalarBackup(string sorting)
		: sortingFunction(sorting)
	{}

	shared_ptr<Node> backup(PriorityQueue<shared_ptr<Node> >& open, vector<shared_ptr<TopLevelAction>>& tlas, shared_ptr<Node> start, unordered_map<State, shared_ptr<Node>, Hash>& closed)
	{
		// First things first, reorder open so it matches our expansion policy needs
		sortOpen(open);

		shared_ptr<Node> goalPrime = open.top();

		// Only move one step towards best on open
		while (goalPrime->getParent() != start)
			goalPrime = goalPrime->getParent();

		return goalPrime;
	}

private:
	void sortOpen(PriorityQueue<shared_ptr<Node> >& open)
	{
		if (sortingFunction == "minimin")
			open.swapComparator(Node::compareNodesF);
		else if (sortingFunction == "bellman")
			open.swapComparator(Node::compareNodesFHat);
	}

protected:
	string sortingFunction;
};
