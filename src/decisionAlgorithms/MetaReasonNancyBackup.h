#pragma once
#include "../utility/DiscreteDistribution.h"
#include "../utility/PriorityQueue.h"
#include "DecisionAlgorithm.h"
#include <functional>
#include <memory>

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
        dijkstraNancyBackup(open, closed_);
        stack<shared_ptr<Node>> commitedNodes;
        closed = closed_;
        prefixDeepThinking(start, commitedNodes);

        if (commitedNodes.empty() && isForceCommit) {
            commitedNodes.push(getAlpha(start));
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
        }

        // initialiizing every node on open to its hhat
        for (auto node : open) {
            node->setBackupHHat(node->getHHatValue());
            node->setNancyFrontier(node);
        }

        // Order open by hhat
        open.swapComparator(Node::compareNodesBackedHHat);

        while (!closedCopy.empty() && !open.empty()) {
            auto cur = open.top();
            open.pop();

            closedCopy.erase(cur->getState());
            auto preds = domain.predecessors(cur->getState());

            for (const auto& s : preds) {
                auto it = closedCopy.find(s);

                if (it == closedCopy.end()) {
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
    }

    void prefixDeepThinking(shared_ptr<Node>         start,
                            stack<shared_ptr<Node>>& commitedNodes)
    {
        auto cur = start;
        int  t   = 0;

        stack<shared_ptr<Node>> reverseOrderedCommitedNodes;
        while (isCommit(cur, t)) {
            reverseOrderedCommitedNodes.push(cur);
            cur = getAlpha(start);
            if (!cur) {
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
        auto alpha = getAlpha(node);
        auto beta  = getBeta(node);

        auto utilityOfCommit    = commitUtility(alpha, timeStep);
        auto utilityOfNotCommit = notCommitUtility(alpha, beta, timeStep);

        return utilityOfCommit > utilityOfNotCommit;
    }

    Cost commitUtility(shared_ptr<Node> alpha, int timeStep)
    {
        auto alphaalpha = getAlpha(alpha);
        auto alphabeta  = getBeta(alpha);

        auto pAlphaalpha =
          distributionAfterSearch(alphaalpha, (timeStep + 1) / 2.0);
        auto pAlphabeta =
          distributionAfterSearch(alphabeta, (timeStep + 1) / 2.0);

        return expectedMinimum(pAlphaalpha, pAlphabeta);
    }

    Cost notCommitUtility(shared_ptr<Node> alpha, shared_ptr<Node> beta,
                          int timeStep)
    {
        auto alphaalpha = getAlpha(alpha);
        auto alphabeta  = getBeta(alpha);
        auto betaalpha  = getAlpha(beta);
        auto betabeta   = getBeta(beta);

        auto pAlphaalpha =
          distributionAfterSearch(alphaalpha, (timeStep / 2.0 + 1) / 2.0);
        auto pAlphabeta =
          distributionAfterSearch(alphabeta, (timeStep / 2.0 + 1) / 2.0);

        auto utilityOfAlpha = expectedMinimum(pAlphaalpha, pAlphabeta);

        auto pBetaalpha =
          distributionAfterSearch(betaalpha, (timeStep / 2.0 + 1) / 2.0);
        auto pBetabeta =
          distributionAfterSearch(betabeta, (timeStep / 2.0 + 1) / 2.0);

        auto utilityOfBeta = expectedMinimum(pBetaalpha, pBetabeta);

        auto pAlpha = distributionAfterSearch(alpha, timeStep / 2.0);
        auto pBeta  = distributionAfterSearch(beta, timeStep / 2.0);

        auto pChooseAlpah = pChoose(pAlpha, pBeta);

        return pChooseAlpah * utilityOfAlpha +
               (1 - pChooseAlpah) * utilityOfBeta;
    }

    shared_ptr<Node> getAlpha(shared_ptr<Node> node)
    {

        vector<State> children = domain.successors(node->getState());

        shared_ptr<Node> bestChild;
        Cost             bestFHat = numeric_limits<double>::infinity();

        for (State child : children) {
            auto it = closed.find(child);

            if (it == closed.end()) {
                continue;
            }

            if (it->second->getParent() != node) {
                continue;
            }

            auto childNode = it->second;

            if (childNode->getNancyFrontier()->getFHatValue() < bestFHat) {
                bestChild = childNode;
                bestFHat  = childNode->getNancyFrontier()->getFHatValue();
            }
        }

        return bestChild;
    }

    // what if there is no beta? just commit?
    shared_ptr<Node> getBeta(shared_ptr<Node> node)
    {
        vector<State> children = domain.successors(node->getState());

        shared_ptr<Node> bestChild;
        shared_ptr<Node> secondBestChild;
        Cost             bestFHat = numeric_limits<double>::infinity();

        for (State child : children) {
            auto it = closed.find(child);

            if (it == closed.end()) {
                continue;
            }

            if (it->second->getParent() != node) {
                continue;
            }

            auto childNode = it->second;

            if (childNode->getNancyFrontier()->getFHatValue() < bestFHat) {
                secondBestChild = bestChild;
                bestChild       = childNode;
                bestFHat        = childNode->getNancyFrontier()->getFHatValue();
            }
        }

        return secondBestChild;
    }

    DiscreteDistribution distributionAfterSearch(shared_ptr<Node> node,
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
                   (1.0 - min(1.0, ds / node->getNancyFrontier()->getDValue()));
        DiscreteDistribution dist(100, mean, var);
        return dist;
    }

    double expectedMinimum(DiscreteDistribution d1, DiscreteDistribution d2)
    {
        double expMin = 0;

        for (const auto& binAlpah : d1) {
            for (const auto& binBeta : d2) {
                expMin = min(binAlpah.cost, binBeta.cost) *
                         binAlpah.probability * binBeta.probability;
            }
        }

        return expMin;
    }

    // probability of choose d1
    double pChoose(DiscreteDistribution d1, DiscreteDistribution d2)
    {
        double prob = 0;

        for (const auto& binAlpah : d1) {
            for (const auto& binBeta : d2) {
                if (binAlpah.cost < binBeta.cost) {
                    prob += binAlpah.probability * binBeta.probability;
                }
            }
        }

        return prob;
    }

    string                                       decisionModule;
    Domain&                                      domain;
    size_t                                       lookahead;
    unordered_map<State, shared_ptr<Node>, Hash> closed;
};
