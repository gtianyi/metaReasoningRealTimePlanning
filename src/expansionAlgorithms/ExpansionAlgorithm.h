#pragma once
#include <unordered_map>
#include <functional>
#include <memory>
#include "../utility/PriorityQueue.h"
#include "../utility/ResultContainer.h"

template <class Domain, class Node, class TopLevelAction>
class ExpansionAlgorithm
{
	typedef typename Domain::State State;
	typedef typename Domain::HashState Hash;
	typedef typename Domain::Cost Cost;

public:
	virtual void incrementLookahead()
	{
	}

	virtual void expand(PriorityQueue<shared_ptr<Node> >& open, unordered_map<State, shared_ptr<Node>, Hash>& closed, vector<shared_ptr<TopLevelAction>>& tlas,
		std::function<bool(shared_ptr<Node>, unordered_map<State, shared_ptr<Node>, Hash>&, PriorityQueue<shared_ptr<Node> >&, vector<shared_ptr<TopLevelAction>>&)> duplicateDetection,
		ResultContainer& res)
	{
	}
};
