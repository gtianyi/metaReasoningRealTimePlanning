#pragma once
//#include "decisionAlgorithms/DecisionAlgorithm.h"
#include "decisionAlgorithms/MetaReasonNancyBackup.h"
#include "decisionAlgorithms/MetaReasonScalarBackup.h"
#include "expansionAlgorithms/MetaReasonAStar.h"
#include "learningAlgorithms/MetaReasonDijkstra.h"
#include "node.h"
#include "utility/PriorityQueue.h"
#include "utility/ResultContainer.h"
#include <functional>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include <cassert>
#include <ctime>

#include "utility/debug.h"

using namespace std;

template<class Domain>
class RealTimeSearch
{
public:
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;
    using Node = SearchNode<Domain>;

    RealTimeSearch(Domain& domain_, string expansionModule_,
                   string decisionModule_, size_t lookahead_)
        : domain(domain_)
        , lookahead(lookahead_)
        , decisionModule(decisionModule_)
    {
        if (decisionModule == "one" || decisionModule == "alltheway" ||
            decisionModule == "dynamicLookahead") {
            metaReasonDecisionAlgo =
              make_shared<MetaReasonScalarBackup<Domain, Node>>(
                decisionModule_);
        } else if (decisionModule == "dtrts" || decisionModule == "dydtrts") {
            metaReasonDecisionAlgo =
              make_shared<MetaReasonNancyBackup<Domain, Node>>(
                decisionModule_, domain, lookahead);
        } else {
            cerr << "unknown decision module: " << decisionModule << "\n";
            exit(1);
        }

        if (expansionModule_ == "astar") {
            metaReasonExpansionAlgo =
              make_shared<MetaReasonAStar<Domain, Node>>(domain, lookahead,
                                                         "f");
        } else if (expansionModule_ == "fhat") {
            metaReasonExpansionAlgo =
              make_shared<MetaReasonAStar<Domain, Node>>(domain, lookahead,
                                                         "fhat");
        } else {
            cerr << "unknown expansion module: " << expansionModule_ << "\n";
            exit(1);
        }

        metaReasonLearningAlgo =
          make_shared<MetaReasonDijkstra<Domain, Node>>(domain);
    }

    ~RealTimeSearch() { clean(); }

    // p: iterationlimit
    ResultContainer search()
    {
        ResultContainer res;

        shared_ptr<Node> initNode =
          make_shared<Node>(0, domain.heuristic(domain.getStartState()),
                            domain.distance(domain.getStartState()),
                            domain.distanceErr(domain.getStartState()), 0, 0, 0,
                            domain.getStartState(), nullptr);

        int count = 0;

        queue<shared_ptr<Node>> actionQueue;
        actionQueue.push(initNode);
        shared_ptr<Node> start = initNode;
        // if set a cutoff iteration
        // while (count <= iterationlimit) {
        while (1) {

            if (decisionModule == "one") {
                start = actionQueue.front();
            }

            if (decisionModule == "alltheway" ||
                decisionModule == "dynamicLookahead" ||
                decisionModule == "dydtrts") {
                vector<string> curPath;
                while (actionQueue.size() > 1) {

                    start = actionQueue.front();
                    actionQueue.pop();
                    curPath.push_back(start->getState().toString());
                    res.solutionLength += 1;
                    res.solutionCost += domain.getEdgeCost(start->getState());

                    // lsslrta* try to optimize cpu time,
                    // so even if more than one action are commited, it
                    // will not use the time to thinking,
                    // so we have to directly advance the "time"
                    res.GATnodesExpanded += lookahead;

                    if (domain.isGoal(start->getState())) {
                        return res;
                    }
                }
                if (!curPath.empty()) {
                    curPath.push_back(
                      actionQueue.front()->getState().toString());
                    res.paths.push_back(curPath);
                }

                start = actionQueue.front();
            }

            // Check if a goal has been reached
            // if yes, just clear the action queue
            if (domain.isGoal(start->getState())) {
                res.solutionFound = true;
                // We stop thinking for metareason approach
                // once the thinking frontier reach a goal
                // ps: this loop should happen only once for one commit
                while (actionQueue.size() > 0) {
                    auto curAction = actionQueue.front();
                    actionQueue.pop();
                    vector<string> curPath;
                    curPath.push_back(curAction->getState().toString());
                    res.paths.push_back(curPath);
                    vector<string> curVisitied;
                    res.visited.push_back(curVisitied);
                    res.isKeepThinkingFlags.push_back(false);
                    vector<string> committed;
                    res.committed.push_back(committed);
                    res.solutionCost +=
                      domain.getEdgeCost(curAction->getState());
                    res.solutionLength += 1;

                    // lsslrta* try to optimize cpu time,
                    // so even if more than one action are commited, it
                    // will not use the time to thinking,
                    // so we have to directly advance the "time"
                    res.GATnodesExpanded += lookahead;

                    if (domain.isGoal(curAction->getState())) {
                        return res;
                    }
                }
            }

            restartLists(start);

            // Expansion and Decision-making Phase
            // check how many of the prefix should be commit
            stack<shared_ptr<Node>> commitQueue;

            // four metaReasoningDecisionAlgo
            // 1. allways commit one, just like old nancy code
            // 2. allways commit to frontier,  modify old nancy code
            //    to return all nodes from root to the best frontier
            // 3. dynamic fhat
            // 4. fhat-pmr: need nancy backup from all frontier and
            //    make decision on whether to commit each prefix based
            //    on the hack rule
            // 5. our approach: compute benefit of doing more search

            // this loop should happen only once for approach 1-3
            int  continueCounter = 0;
            bool keepThinking    = false;
            bool goalReached     = false;

            while (commitQueue.empty() && !actionQueue.empty() &&
                   !goalReached) {
                // do more search
                metaReasonExpansionAlgo->expand(open, closed,
                                                duplicateDetection, res);
                // deadend
                if (open.empty()) {
                    break;
                }

                if (domain.isGoal(open.top()->getState())) {
                    metaReasonDecisionAlgo =
                      make_shared<MetaReasonScalarBackup<Domain, Node>>(
                        "alltheway");
                    goalReached = true;
                }

                // meta-reason about how much to commit
                commitQueue =
                  metaReasonDecisionAlgo->backup(open, start, closed, false);

                DEBUG_MSG("start, ");
                DEBUG_MSG(start->toString());
                DEBUG_MSG("continue search: " << continueCounter);
                DEBUG_MSG("commit size: " << commitQueue.size());
                DEBUG_MSG("actionQ size: " << actionQueue.size());

                auto n = actionQueue.front();
                actionQueue.pop();
                if (decisionModule != "alltheway" &&
                    decisionModule != "dynamicLookahead" &&
                    decisionModule != "dydtrts") {
                    vector<string> curPath;
                    curPath.push_back(n->getState().toString());
                    res.paths.push_back(curPath);
                    res.isKeepThinkingFlags.push_back(keepThinking);
                    keepThinking = true;
                }

                res.solutionCost += domain.getEdgeCost(n->getState());
                res.solutionLength += 1;
                ++continueCounter;
                if (commitQueue.empty() && !actionQueue.empty()) {
                    vector<string> committed;
                    res.committed.push_back(committed);
                }
            }

            // deadend
            if (open.empty()) {
                DEBUG_MSG("deadend!");
                break;
            }

            // if action queue is empty and metareasoning do not want to commit
            // force to commit at least one action
            if (commitQueue.empty()) {
                // force to commit at least one action
                commitQueue =
                  metaReasonDecisionAlgo->backup(open, start, closed, true);

                DEBUG_MSG("force commit, commit queue size "
                          << commitQueue.size());
            }

            assert(commitQueue.size() > 0);
            if (decisionModule == "dynamicLookahead" ||
                decisionModule == "dydtrts") {
                metaReasonExpansionAlgo->increaseLookahead(lookahead *
                                                           commitQueue.size());
            }
            vector<string> commited;
            while (!commitQueue.empty()) {
                auto n = commitQueue.top();
                DEBUG_MSG("commit: " << n->toString());
                commitQueue.pop();
                actionQueue.push(n);
                start = n;
                commited.push_back(n->getState().toString());
            }
            res.committed.push_back(commited);

            // LearninH Phase
            metaReasonLearningAlgo->learn(open, closed);

            ++count;
            DEBUG_MSG("iteration: " << count);
        }

        return res;
    }

private:
    static bool duplicateDetection(
      shared_ptr<Node>                              node,
      unordered_map<State, shared_ptr<Node>, Hash>& closed,
      PriorityQueue<shared_ptr<Node>>&              open)
    {
        // Check if this state exists
        auto it = closed.find(node->getState());

        if (it != closed.end()) {
            // This state has been generated before, check if its node is on
            // OPEN
            if (it->second->onOpen()) {
                // This node is on OPEN, keep the better g-value
                if (node->getGValue() < it->second->getGValue()) {
                    it->second->setGValue(node->getGValue());
                    it->second->setParent(node->getParent());
                    it->second->setHValue(node->getHValue());
                    it->second->setDValue(node->getDValue());
                    it->second->setDErrValue(node->getDErrValue());
                    it->second->setEpsilonH(node->getPathBasedEpsilonH());
                    it->second->setEpsilonD(node->getPathBasedEpsilonD());
                    it->second->setState(node->getState());
                    open.update(it->second);
                }
            } else {
                // This node is on CLOSED, compare the f-values. If this new
                // f-value is better, reset g, h, and d.
                // Then reopen the node.
                if (node->getFValue() < it->second->getFValue()) {
                    it->second->setGValue(node->getGValue());
                    it->second->setParent(node->getParent());
                    it->second->setHValue(node->getHValue());
                    it->second->setDValue(node->getDValue());
                    it->second->setDErrValue(node->getDErrValue());
                    it->second->setEpsilonH(node->getPathBasedEpsilonH());
                    it->second->setEpsilonD(node->getPathBasedEpsilonD());
                    it->second->setState(node->getState());
                    it->second->reOpen();
                    open.push(it->second);
                }
            }

            return true;
        }

        return false;
    }

    void restartLists(shared_ptr<Node> start)
    {
        // mark this node as the start of the current search (to
        // prevent state pruning based on label)
        start->markStart();
        start->resetStartEpsilons();

        // Empty OPEN and CLOSED
        open.clear();

        // delete all of the nodes from the last expansion phase
        closed.clear();

        // reset start g as 0
        start->setGValue(0);

        start->setParent(nullptr);

        open.push(start);
        closed[start->getState()] = start;
    }

    void clean()
    {
        // Empty OPEN and CLOSED
        open.clear();

        // delete all of the nodes from the last expansion phase
        closed.clear();
    }

    void noSolutionFound(ResultContainer& res)
    {
        res.solutionFound = false;
        res.solutionCost  = -1;
    }

protected:
    Domain&                                      domain;
    shared_ptr<DecisionAlgorithm<Domain, Node>>  metaReasonDecisionAlgo;
    shared_ptr<MetaReasonAStar<Domain, Node>>    metaReasonExpansionAlgo;
    shared_ptr<MetaReasonDijkstra<Domain, Node>> metaReasonLearningAlgo;
    PriorityQueue<shared_ptr<Node>>              open;
    unordered_map<State, shared_ptr<Node>, Hash> closed;

    size_t lookahead;
    string decisionModule;
};
