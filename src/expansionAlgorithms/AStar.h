#pragma once
#include "../utility/PriorityQueue.h"
#include "../utility/ResultContainer.h"
#include "ExpansionAlgorithm.h"
#include <functional>
#include <memory>
#include <unordered_map>

using namespace std;

template<class Domain, class Node, class TopLevelAction>
class AStar : public ExpansionAlgorithm<Domain, Node, TopLevelAction>
{
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;

public:
    AStar(Domain& domain_, double lookahead_, string sorting_)
        : domain(domain_)
        , lookahead(lookahead_)
        , sortingFunction(sorting_)
    {}

    void incrementLookahead() { lookahead++; }

    void expand(
      PriorityQueue<shared_ptr<Node>>&              open,
      unordered_map<State, shared_ptr<Node>, Hash>& closed,
      vector<shared_ptr<TopLevelAction>>&           tlas,
      std::function<bool(
        shared_ptr<Node>, unordered_map<State, shared_ptr<Node>, Hash>&,
        PriorityQueue<shared_ptr<Node>>&, vector<shared_ptr<TopLevelAction>>&)>
                       duplicateDetection,
      ResultContainer& res)
    {
        // First things first, reorder open so it matches our expansion policy
        // needs
        sortOpen(open);

        // This starts at 1, because we had to expand start to get the top level
        // actions
        int expansions = 1;

        // Expand until the limit
        while (!open.empty() && (expansions < lookahead)) {
            // Pop lowest fhat-value off open
            shared_ptr<Node> cur = open.top();

            // Check if current node is goal
            if (domain.isGoal(cur->getState())) {
                return;
            }

            res.nodesExpanded++;
            expansions++;

            open.pop();
            cur->close();

            // Remove this node from the open list of any TLAs
            tlas[cur->getOwningTLA()]->open_TLA.remove(cur);

            vector<State> children = domain.successors(cur->getState());
            res.nodesGenerated += children.size();

            State bestChild;
            Cost  bestF = numeric_limits<double>::infinity();

            for (State child : children) {
                shared_ptr<Node> childNode = make_shared<Node>(
                  cur->getGValue() + domain.getEdgeCost(child),
                  domain.heuristic(child), domain.distance(child),
                  domain.distanceErr(child), domain.epsilonHGlobal(),
                  domain.epsilonDGlobal(), child, cur, cur->getOwningTLA());

                bool dup = duplicateDetection(childNode, closed, open, tlas);

                if (!dup && childNode->getFValue() < bestF) {
                    bestF     = childNode->getFValue();
                    bestChild = child;
                }

                // Duplicate detection
                if (!dup) {
                    open.push(childNode);
                    closed[child] = childNode;

                    // Add to open of generating TLA
                    tlas[childNode->getOwningTLA()]->open_TLA.push(childNode);
                }
            }

            // Learn one-step error
            if (bestF != numeric_limits<double>::infinity()) {
                Cost epsD = (1 + domain.distance(bestChild)) - cur->getDValue();
                Cost epsH = (domain.getEdgeCost(bestChild) +
                             domain.heuristic(bestChild)) -
                            cur->getHValue();

                domain.pushEpsilonHGlobal(epsH);
                domain.pushEpsilonDGlobal(epsD);
            }
        }
    }

private:
    void sortOpen(PriorityQueue<shared_ptr<Node>>& open)
    {
        if (sortingFunction == "f")
            open.swapComparator(Node::compareNodesF);
        else if (sortingFunction == "fhat")
            open.swapComparator(Node::compareNodesFHat);
        else if (sortingFunction == "lowerconfidence")
            open.swapComparator(Node::compareNodesLC);
    }

protected:
    Domain& domain;
    double  lookahead;
    string  sortingFunction;
};
