#pragma once
#include "../utility/PriorityQueue.h"
#include "../utility/debug.h"
#include <functional>
#include <memory>
#include <unordered_map>

using namespace std;

template<class Domain, class Node>
class MetaReasonDijkstra 
{
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;

public:
    MetaReasonDijkstra(Domain& domain_)
        : domain(domain_)
    {}

    void learn(PriorityQueue<shared_ptr<Node>>              open,
               unordered_map<State, shared_ptr<Node>, Hash> closed)
    {
        // Start by initializing every state in closed to inf h
        for (typename unordered_map<State, shared_ptr<Node>, Hash>::iterator
               it = closed.begin();
             it != closed.end(); it++) {
            if (!it->second->onOpen())
                domain.updateHeuristic(it->first,
                                       numeric_limits<double>::infinity());
        }

        // Order open by h
        open.swapComparator(Node::compareNodesHHat);

        // Perform reverse dijkstra while closed is not empy
        while (!closed.empty() && !open.empty()) {
            shared_ptr<Node> cur = open.top();
            open.pop();

            closed.erase(cur->getState());
            auto preds = domain.predecessors(cur->getState());
            // DEBUG_MSG("open state: "<<cur->getState()<<"pred size: "<<
            // preds.size());

            // Now get all of the predecessors of cur
            for (const State s : domain.predecessors(cur->getState())) {
                // DEBUG_MSG("learning state: "<<s);

                typename unordered_map<State, shared_ptr<Node>, Hash>::iterator
                  it = closed.find(s);

                if (it != closed.end() &&
                    cur->getParent() == it->second && 
                    domain.heuristic(s) > domain.getEdgeCost(cur->getState()) +
                                            domain.heuristic(cur->getState())) {
                    // Update the heuristic of this pedecessor
                    domain.updateHeuristic(s,
                                           domain.getEdgeCost(cur->getState()) +
                                             domain.heuristic(cur->getState()));
                    // Update the distance of this predecessor
                    domain.updateDistance(s,
                                          domain.distance(cur->getState()) + 1);
                    // Update the distance for the heuristic error of this
                    // predecessor
                    domain.updateDistanceErr(
                      s, domain.distanceErr(cur->getState()));

                    it->second->setDValue(domain.distance(s));
                    it->second->setDErrValue(domain.distanceErr(s));
                    it->second->setHValue(domain.heuristic(s));
                    //it->second->setEpsilonH(cur->getPathBasedEpsilonH());
                    it->second->setEpsilonH(cur->getEpsilonH());
                    //it->second->setEpsilonD(cur->getPathBasedEpsilonD());
                    it->second->setEpsilonD(cur->getEpsilonD());

                    if (open.find(it->second) == open.end()) {
                        open.push(it->second);
                    } else {
                        open.update(it->second);
                    }
                }
            }
        }
    }

protected:
    Domain& domain;
};
