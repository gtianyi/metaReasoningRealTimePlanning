#pragma once
#include "../utility/NormalDistribution.h"
#include "../utility/PriorityQueue.h"
#include "../utility/debug.h"
#include "DecisionAlgorithm.h"
#include <cassert>
#include <functional>
#include <memory>
#include <sstream>

using namespace std;

template<class Domain, class Node>
class MetaReasonNancyBackup : public DecisionAlgorithm<Domain, Node>
{
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;

public:
    MetaReasonNancyBackup(string& decisionModule_, Domain& domain_,
                          size_t lookahead_)
        : decisionModule(decisionModule_)
        , domain(domain_)
        , lookahead(lookahead_)
    {}

    stack<shared_ptr<Node>> backup(
      const PriorityQueue<shared_ptr<Node>>& open, shared_ptr<Node> start,
      const unordered_map<State, shared_ptr<Node>, Hash>& closed_,
      const bool                                          isForceCommit)
    {
        closed = closed_;
        dijkstraNancyBackup(open, closed_);
        stack<shared_ptr<Node>> commitedNodes;
        prefixDeepThinking(start, commitedNodes);

        if (commitedNodes.empty() && isForceCommit) {
            // DEBUG_MSG("force commit");
            auto alpha = getAlpha(start);
            if (alpha == nullptr) {
                // DEBUG_MSG("alpha is null");
                return commitedNodes;
            }
            commitedNodes.push(alpha);
        }

        return commitedNodes;
    }

protected:
    void dijkstraNancyBackup(
      PriorityQueue<shared_ptr<Node>>              open,
      unordered_map<State, shared_ptr<Node>, Hash> closedCopy)
    {
        // Start by initializing every state in closed to inf hhat
        for (auto it = closedCopy.begin(); it != closedCopy.end(); it++) {
            it->second->setBackupHHat(numeric_limits<double>::infinity());
            // DEBUG_MSG("dijksttra closed:" + it->second->toString());
        }

        // initialiizing every node on open to its hhat
        for (auto node : open) {
            node->setBackupHHat(node->getHHatValue());
            node->setNancyFrontier(node);
            // DEBUG_MSG("dijksttra open:" + node->toString());
        }

        // Order open by hhat
        open.swapComparator(Node::compareNodesBackedHHat);

        while (!open.empty()) {
            auto cur = open.top();
            open.pop();
            // DEBUG_MSG("dijksttra state" + cur->toString());

            closedCopy.erase(cur->getState());
            auto preds = domain.predecessors(cur->getState());

            for (const auto& s : preds) {
                auto it = closedCopy.find(s);

                if (it == closedCopy.end() || it->second != cur->getParent()) {
                    continue;
                }

                auto parentNode = it->second;
                auto edgeCost   = domain.getEdgeCost(cur->getState());

                if (parentNode->getBackupHHatValue() <=
                    edgeCost + cur->getBackupHHatValue()) {
                    continue;
                }

                parentNode->setBackupHHat(edgeCost + cur->getBackupHHatValue());
                parentNode->setNancyFrontier(cur->getNancyFrontier());

                if (open.find(parentNode) == open.end()) {
                    open.push(parentNode);
                } else {
                    open.update(parentNode);
                }
            }
        }

        // everything else in the closed is deadend
        for (auto it = closedCopy.begin(); it != closedCopy.end(); it++) {
            it->second->setHValue(numeric_limits<double>::infinity());
            it->second->setDValue(numeric_limits<double>::infinity());
            it->second->setDErrValue(numeric_limits<double>::infinity());
            it->second->setEpsilonH(0);
            it->second->setEpsilonD(0);
            it->second->setNancyFrontier(it->second);
        }
    }

    void prefixDeepThinking(shared_ptr<Node>         start,
                            stack<shared_ptr<Node>>& commitedNodes)
    {
        auto cur = start;
        int  t   = 1;

        stack<shared_ptr<Node>> reverseOrderedCommitedNodes;

        while (isCommit(cur, t)) {
            auto curAlpha = getAlpha(cur);
            reverseOrderedCommitedNodes.push(curAlpha);
            cur = curAlpha;
            if (cur == nullptr) {
                break;
            }
            t += 1;
        }

        while (!reverseOrderedCommitedNodes.empty()) {
            commitedNodes.push(reverseOrderedCommitedNodes.top());
            reverseOrderedCommitedNodes.pop();
        }
    }

    bool isCommit(shared_ptr<Node> node, int timeStep)
    {
        DEBUG_MSG("meta reasoning========== timestep " + to_string(timeStep));
        auto alpha = getAlpha(node);
        auto beta  = getBeta(node);

        DEBUG_MSG("cur " + node->toString());
        if (alpha == nullptr && beta == nullptr) {
            // this is the frontier, so not commit
            // we need grandchilden info to make meta decision
            // so we always choose to not commit the very most frontier
            // that has no children generated yet.
            // lookahead has to be at least 2,
            // otherwise it will always choose to not commit
            DEBUG_MSG("both a and b are null, return false ");
            return false;
        }

        if (beta == nullptr) {
            DEBUG_MSG("b is null, return true");
            return true;
        }

        DEBUG_MSG("alpha " + alpha->toString());
        DEBUG_MSG("alpha frontier" + alpha->getNancyFrontier()->toString());
        DEBUG_MSG("beta " + beta->toString());
        DEBUG_MSG("beta frontier" + beta->getNancyFrontier()->toString());

        auto pChooseAlpha = getPChooseAlpha(alpha, beta, timeStep);

        if (pChooseAlpha == 1.0) {
            DEBUG_MSG("p_a is 1, return true");
            return true;
        }

        auto utilityOfCommit = commitUtility(alpha, timeStep);
        auto utilityOfNotCommit =
          notCommitUtility(alpha, beta, pChooseAlpha, timeStep);

        DEBUG_MSG("uCMT " + to_string(utilityOfCommit) + " uNCMT " +
                  to_string(utilityOfNotCommit));
        return utilityOfCommit < utilityOfNotCommit;
    }

    double getPChooseAlpha(shared_ptr<Node> alpha, shared_ptr<Node> beta,
                           int timeStep)
    {
        auto pAlpha = distributionAfterSearch(alpha, timeStep / 2.0);
        auto pBeta  = distributionAfterSearch(beta, timeStep / 2.0);

        return computeProbOfd1IsLowerCost(pAlpha, pBeta);
    }

    Cost commitUtility(shared_ptr<Node> alpha, int timeStep)
    {
        auto alphaalpha = getAlpha(alpha);
        auto alphabeta  = getBeta(alpha);

        if (alphaalpha == nullptr && alphabeta == nullptr) {
             DEBUG_MSG("no alphaAlpha and alphaBeta");
            return alpha->getNancyFrontier()->getFHatValue();
        }

        if (alphabeta == nullptr) {
             DEBUG_MSG("no alphaBeta");
            return alphaalpha->getNancyFrontier()->getFHatValue();
        }

        DEBUG_MSG("aa "<<alphaalpha->toString());
        DEBUG_MSG("ab "<<alphabeta->toString());

        auto pAlphaalpha =
          distributionAfterSearch(alphaalpha, (timeStep + 1) / 2.0);
        auto pAlphabeta =
          distributionAfterSearch(alphabeta, (timeStep + 1) / 2.0);
        DEBUG_MSG("baa"<<pAlphaalpha.toString());
        DEBUG_MSG("bab"<<pAlphabeta.toString());

        return expectedMinimum(pAlphaalpha, pAlphabeta);
    }

    Cost notCommitUtility(shared_ptr<Node> alpha, shared_ptr<Node> beta,
                          double pChooseAlpha, int timeStep)
    {
        auto alphaalpha = getAlpha(alpha);
        auto alphabeta  = getBeta(alpha);
        auto betaalpha  = getAlpha(beta);
        auto betabeta   = getBeta(beta);

        double utilityOfAlpha = alpha->getNancyFrontier()->getFHatValue();

        if (alphaalpha != nullptr && alphabeta != nullptr) {
            // DEBUG_MSG("has both kids for alpha, in nCMT");
            auto pAlphaalpha =
              distributionAfterSearch(alphaalpha, (timeStep / 2.0 + 1) / 2.0);
            auto pAlphabeta =
              distributionAfterSearch(alphabeta, (timeStep / 2.0 + 1) / 2.0);

            utilityOfAlpha = expectedMinimum(pAlphaalpha, pAlphabeta);
        }

        auto utilityOfBeta = beta->getNancyFrontier()->getFHatValue();

        if (betaalpha != nullptr && betabeta != nullptr) {
            // DEBUG_MSG("has both kids for beta, in nCMT");
            auto pBetaalpha =
              distributionAfterSearch(betaalpha, (timeStep / 2.0 + 1) / 2.0);
            auto pBetabeta =
              distributionAfterSearch(betabeta, (timeStep / 2.0 + 1) / 2.0);

            utilityOfBeta = expectedMinimum(pBetaalpha, pBetabeta);
        }

        // DEBUG_MSG("uA " + to_string(utilityOfAlpha));
        // DEBUG_MSG("uB " + to_string(utilityOfBeta));
        // DEBUG_MSG("pChooseAlpha " + to_string(pChooseAlpha));

        return pChooseAlpha * utilityOfAlpha +
               (1 - pChooseAlpha) * utilityOfBeta;
    }

    shared_ptr<Node> getAlpha(shared_ptr<Node> node)
    {
        vector<State> children = domain.successors(node->getState());

        shared_ptr<Node> bestChild;
        Cost             bestFHat = numeric_limits<double>::infinity();

        //DEBUG_MSG("getAlph on state " + node->toString());
        for (State child : children) {
            auto it = closed.find(child);

            if (it == closed.end()) {
                continue;
            }

            if (it->second->getParent() != node) {
                continue;
            }

            auto childNode = it->second;

            //DEBUG_MSG("getAlphh work on kid " + childNode->toString());
            //DEBUG_MSG("getAlphh work on kid's nancyfront" + childNode->getNancyFrontier()->toString());
            if ((bestChild &&
                 childNode->getNancyFrontier()->getFHatValue() == bestFHat &&
                 // break tie on high g
                 childNode->getNancyFrontier()->getGValue() >
                   bestChild->getNancyFrontier()->getGValue()) ||
                (childNode->getNancyFrontier()->getFHatValue() < bestFHat)) {
                bestChild = childNode;
                bestFHat  = childNode->getNancyFrontier()->getFHatValue();

                //DEBUG_MSG("is best and best fhat " << bestFHat);
            }
        }

        if (bestChild != nullptr) {
            assert(bestChild != node);
        }

        return bestChild;
    }

    // what if there is no beta? just commit?
    shared_ptr<Node> getBeta(shared_ptr<Node> node)
    {
        vector<State> children = domain.successors(node->getState());

        shared_ptr<Node> bestChild = getAlpha(node);
        shared_ptr<Node> secondBestChild;
        Cost             secondBestFHat = numeric_limits<double>::infinity();

        for (State child : children) {
            auto it = closed.find(child);

            if (it == closed.end()) {
                continue;
            }

            if (it->second->getParent() != node) {
                continue;
            }

            auto childNode = it->second;

            if (childNode->getNancyFrontier()->getFHatValue() <
                  secondBestFHat &&
                childNode != bestChild) {
                secondBestChild = childNode;
                secondBestFHat  = childNode->getNancyFrontier()->getFHatValue();
            }
        }

        if (secondBestChild != nullptr) {
            // DEBUG_MSG("find beta");
            assert(secondBestChild != node);
            assert(secondBestChild != bestChild);
        }

        return secondBestChild;
    }

    NormalDistribution distributionAfterSearch(shared_ptr<Node> node,
                                               double timeStepFraction)
    {
        auto mean = node->getNancyFrontier()->getFHatValue();
        // Just use global delay
        // version 2 try path based expansion delay?
        double ds =
          // for identity action
          timeStepFraction * static_cast<double>(lookahead) /
          domain.averageDelayWindow();
        // for Slo'RTS
        // node->getDValue();

        // we are using path-based heuristic error here
        auto var = pow(node->getNancyFrontier()->getPathBasedEpsilonH() *
                         node->getNancyFrontier()->getDValue(),
                       2.0) *
                   min(1.0, ds / node->getNancyFrontier()->getDValue());

        // DEBUG_MSG("epsilonh " +
        // to_string(node->getNancyFrontier()->getPathBasedEpsilonH()));
        // DEBUG_MSG("ds " + to_string(ds));
        // DEBUG_MSG("dab " + to_string(node->getNancyFrontier()->getDValue()));
        // DEBUG_MSG("mean " + to_string(mean) + " var " + to_string(var));
        return NormalDistribution(mean, var);
    }

    double expectedMinimum(const NormalDistribution& d1,
                           const NormalDistribution& d2)
    {

        if (d1.getVar() < 0.01 && d2.getVar() < 0.01){
            return min(d1.getMean(), d2.getMean());
        }

        auto theta = sqrt(d1.getVar() + d2.getVar());

        auto x = (d1.getMean() - d2.getMean()) / theta;

        auto expMin = d1.getMean() * standardNormalCDF(x) +
                      d2.getMean() * standardNormalCDF(x) -
                      theta * standardNormalPDF(x);

        // DEBUG_MSG("exp min: " + to_string(expMin));

        return expMin;
    }

    string                                       decisionModule;
    Domain&                                      domain;
    size_t                                       lookahead;
    unordered_map<State, shared_ptr<Node>, Hash> closed;
};
