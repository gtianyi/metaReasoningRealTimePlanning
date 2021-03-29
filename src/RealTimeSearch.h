#pragma once
//#include "decisionAlgorithms/DecisionAlgorithm.h"
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
    using Node = Node<Domain>;

    RealTimeSearch(Domain& domain_, string decisionModule_, size_t lookahead_)
        : domain(domain_)
        , lookahead(lookahead_)
        , decisionModule(decisionModule_)
    {
        if (decisionModule == "one" || decisionModule == "alltheway") {
            metaReasonDecisionAlgo =
              make_shared<MetaReasonScalarBackup<Domain, Node>>(
                decisionModule_);
        } else if (decisionModule == "dtrts") {
            metaReasonDecisionAlgo =
              make_shared<MetaReasonNancyBackup<Domain, Node>>(
                decisionModule_, domain, lookahead);
        } else {
            cerr << "unknown decision module: " << decisionModule << "\n";
            exit(1);
        }

        metaReasonExpansionAlgo =
          make_shared<MetaReasonAStar<Domain, Node>>(domain, lookahead, "f");

        metaReasonLearningAlgo =
          make_shared<MetaReasonDijkstra<Domain, Node>>(domain);
    }

    ~RealTimeSearch() { clean(); }

    // p: iterationlimit
    ResultContainer search()
    {
        ResultContainer res;

        shared_ptr<Node> initNode = make_shared<Node>(
          0, domain.heuristic(domain.getStartState()),
          domain.distance(domain.getStartState()),
          domain.distanceErr(domain.getStartState()), domain.epsilonHGlobal(),
          domain.epsilonDGlobal(), domain.getStartState(), nullptr);

        int count = 0;

        queue<shared_ptr<Node>> actionQueue;
        actionQueue.push(initNode);

        // if set a cutoff iteration
        // while (count <= iterationlimit) {
        while (1) {
            auto start = actionQueue.front();

            if (decisionModule == "alltheway") {
                vector<string> curPath;
                while (actionQueue.size() > 1) {

                    start = actionQueue.front();
                    actionQueue.pop();
                    curPath.push_back(start->getState().toString());
                    // TODO cost can not be computed here
                    // res.solutionCost += n->getGValue();
                    res.solutionLength += 1;

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
            if (domain.isGoal(start->getState())) {
                res.solutionFound = true;
                // vector<string> curPath;
                // curPath.push_back(start->getState().toString());
                // res.paths.push_back(curPath);

                res.solutionLength += 1;

                return res;
            }

            restartLists(start);

            domain.updateEpsilons();

            // Expansion and Decision-making Phase
            // check how many of the prefix should be commit
            stack<shared_ptr<Node>> commitQueue;

            // four metaReasoningDecisionAlgo
            // 1. allways commit one, just like old nancy code
            // 2. allways commit to frontier,  modify old nancy code
            //    to return all nodes from root to the best frontier
            // 3. fhat-pmr: need nancy backup from all frontier and
            //    make decision on whether to commit each prefix based
            //    on the hack rule
            // 4. our approach: compute benefit of doing more search

            // this loop should happen only once for approach 1-3
            while (commitQueue.empty() && !actionQueue.empty()) {
                // do more search
                metaReasonExpansionAlgo->expand(open, closed,
                                                duplicateDetection, res);
                // deadend
                if (open.empty()) {
                    break;
                }

                // meta-reason about how much to commit
                commitQueue =
                  metaReasonDecisionAlgo->backup(open, start, closed, false);

                DEBUG_MSG("commit size: " << commitQueue.size());

                auto n = actionQueue.front();
                actionQueue.pop();
                if (decisionModule != "alltheway") {
                    vector<string> curPath;
                    curPath.push_back(n->getState().toString());
                    res.paths.push_back(curPath);
                }
                // TODO cost can not be computed here
                // res.solutionCost += n->getGValue();
                res.solutionLength += 1;
            }

            // deadend
            if (open.empty()) {
                break;
            }

            // if action queue is empty and metareasoning do not want to commit
            // force to commit at least one action
            if (commitQueue.empty()) {
                // force to commit at least one action
                commitQueue =
                  metaReasonDecisionAlgo->backup(open, start, closed, true);
            }

            assert(commitQueue.size() > 0);

            while (!commitQueue.empty()) {
                auto n = commitQueue.top();
                commitQueue.pop();
                actionQueue.push(n);
            }

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
        typename unordered_map<State, shared_ptr<Node>, Hash>::iterator it =
          closed.find(node->getState());

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
                    it->second->setEpsilonH(node->getEpsilonH());
                    it->second->setEpsilonD(node->getEpsilonD());
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
                    it->second->setEpsilonH(node->getEpsilonH());
                    it->second->setEpsilonD(node->getEpsilonD());
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

        // Empty OPEN and CLOSED
        open.clear();

        // delete all of the nodes from the last expansion phase
        closed.clear();

        // reset start g as 0
        start->setGValue(0);

        start->setParent(nullptr);

        open.push(start);
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
