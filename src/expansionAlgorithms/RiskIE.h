#pragma once
#include <unordered_map>
#include <functional>
#include <limits>
#include <memory>
#include "ExpansionAlgorithm.h"
#include "../utility/PriorityQueue.h"
#include"../utility/ResultContainer.h"

using namespace std;

template <class Domain, class Node, class TopLevelAction>
class RiskIE : public ExpansionAlgorithm<Domain, Node, TopLevelAction>
{
	typedef typename Domain::State State;
	typedef typename Domain::Cost Cost;
	typedef typename Domain::HashState Hash;

public:
    RiskIE(Domain& domain, double lookahead)
            : domain(domain), lookahead(lookahead) {}

    void incrementLookahead() { lookahead++; }

    void expand(PriorityQueue<shared_ptr<Node>>& open,
            unordered_map<State, shared_ptr<Node>, Hash>& closed,
            vector<shared_ptr<TopLevelAction>>& tlas,
            std::function<bool(shared_ptr<Node>,
                    unordered_map<State, shared_ptr<Node>, Hash>&,
                    PriorityQueue<shared_ptr<Node>>&,
                    vector<shared_ptr<TopLevelAction>>&)> duplicateDetection,
            ResultContainer& res) {
        // This starts at 1, because we had to expand start to get the top level
        // actions
        int expansions = 1;

        while (expansions < lookahead && !open.empty()) {
            // At the beginning of each expansion we are going to update our
            // beliefs at every TLA.
            // Why? Because this could be the first expansion in the phase.
            // If it isn't? Wouldn't we only need to update the TLA of the node
            // we expanded last iteration?
            // In a tree, yes. But in a graph, this could have placed nodes onto
            // the open lists of other TLAs.
            // Therefore, the beliefs of all TLAs should be updated before every
            // expansion.
            kBestDecision(tlas);

            // Simulate expansion of best node under each TLA
            int chosenTLAIndex = intervalEstimation(tlas);

            // Expand under the TLA which holds the lowest lower confidence bar 
            shared_ptr<Node> chosenNode = tlas[chosenTLAIndex]->open_TLA.top();

            // Add this node to the expansion delay window
            domain.pushDelayWindow(chosenNode->getDelayCntr());

            // Check if current node is goal. If it is, then the expansion phase
            // is over, time to move.
            if (domain.isGoal(chosenNode->getState())) {
                return;
            }

            // Remove the chosen node from open
            tlas[chosenTLAIndex]->open_TLA.pop();
            open.remove(chosenNode);
            chosenNode->close();

            // Book keeping for expansion count
            res.nodesExpanded++;
            expansions++;

            // Increment the delay counts of every other node on open...
            for (shared_ptr<Node> n : open) {
                n->incDelayCntr();
            }

            // Generate the successors of the chosen node
            vector<State> children = domain.successors(chosenNode->getState());

            // Book keeping for number of nodes generated
            res.nodesGenerated += children.size();

            State bestChild;
            Cost bestF = numeric_limits<double>::infinity();

            // Iterate over the successor states
            for (State child : children) {
                // Create a node for this state
                shared_ptr<Node> childNode = make_shared<Node>(
                        chosenNode->getGValue() + domain.getEdgeCost(child),
                        domain.heuristic(child),
                        domain.distance(child),
                        domain.distanceErr(child),
                        domain.epsilonHGlobal(),
                        domain.epsilonDGlobal(),
                        child,
                        chosenNode,
                        chosenNode->getOwningTLA());

                bool dup = duplicateDetection(childNode, closed, open, tlas);

                if (!dup && childNode->getFValue() < bestF) {
                    bestF = childNode->getFValue();
                    bestChild = child;
                }

                // Duplicate detection performed
                if (!dup) {
                    // If this state hasn't yet been reached, add this node open
                    open.push(childNode);
                    closed[child] = childNode;

                    // Add to open of generating TLA
                    tlas[chosenTLAIndex]->open_TLA.push(childNode);
                }
            }

            // Learn the one-step error
            if (bestF != numeric_limits<double>::infinity()) {
                Cost epsD = (1 + domain.distance(bestChild)) -
                        chosenNode->getDValue();
                Cost epsH = (domain.getEdgeCost(bestChild) +
                                    domain.heuristic(bestChild)) -
                        chosenNode->getHValue();

                domain.pushEpsilonHGlobal(epsH);
                domain.pushEpsilonDGlobal(epsD);
            }
        }
	}

private:
	int intervalEstimation(vector<shared_ptr<TopLevelAction>>& tlas)
	{
		int minimal_LC_TLA = 0;
		double minimal_LC = numeric_limits<double>::infinity();

		for (int i = 1; i < tlas.size(); i++)
		{
		    auto lc = Node::getLowerConfidence(tlas[i]->topLevelNode);

			if (lc < minimal_LC)
			{
				minimal_LC =lc;
				minimal_LC_TLA = i;
			}
		}

		return minimal_LC_TLA;
	}

	void csernaBackup(shared_ptr<TopLevelAction> tla)
	{
		// We assume in k-best that only the k-best nodes matter.
		if (!tla->kBestNodes.empty())
		{
			// Perform Cserna Backups on k-best nodes
			while (tla->kBestNodes.size() > 1)
			{
				// Take the first two and do a Cserna backup...
				tla->kBestNodes[0]->distribution = tla->kBestNodes[0]->distribution * tla->kBestNodes[1]->distribution;
				// Remove the other used in the backup
				tla->kBestNodes.erase(remove(tla->kBestNodes.begin(), tla->kBestNodes.end(), tla->kBestNodes[1]), tla->kBestNodes.end());
			}

			// Get the expected value of the resulting Cserna Distribution
			tla->expectedMinimumPathCost = tla->kBestNodes[0]->distribution.expectedCost();
			tla->setBelief(tla->kBestNodes[0]->distribution);
		}
		else
		{
			// If nothing was expanded under this TLA, use the expected value of the TLA
			tla->expectedMinimumPathCost = numeric_limits<double>::infinity();
		}
	}

	void kBestDecision(vector<shared_ptr<TopLevelAction>>& tlas)
	{
		// The K-Best decision assumes that the only nodes within the subtrees of the TLAs are the k-best frontier nodes
		// on their opened lists. Find them.
		for (auto tla : tlas)
		{
			tla->kBestNodes.clear();

			// If this TLA has unique, probably optimal subtrees beneath it, it is valid

			int i = 0;
			// Add to the best k nodes while i < k and non-selected nodes exist on the frontier
			while (i < k && !tla->open_TLA.empty())
			{
				shared_ptr<Node> best = tla->open_TLA.top();
				tla->open_TLA.pop();

				// Make this node's PDF a discrete distribution...
				best->distribution = DiscreteDistribution(100, best->getFValue(), best->getFHatValue(),
					best->getDValue(), best->getFHatValue() - best->getFValue());

				tla->kBestNodes.push_back(best);
				i++;
			}

			// Now put the nodes back in the top level open list
			for (shared_ptr<Node> n : tla->kBestNodes)
			{
				tla->open_TLA.push(n);
			}

			// Now that k-best are selected, perform Cserna backup
			csernaBackup(tla);
		}
	}

protected:
	Domain & domain;
	double lookahead;
	int k = 1;
};
