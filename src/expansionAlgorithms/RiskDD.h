#pragma once
#include <functional>
#include <limits>
#include <memory>
#include <unordered_map>
#include "../utility/PriorityQueue.h"
#include "../utility/ResultContainer.h"
#include "ExpansionAlgorithm.h"
#include "../decisionAlgorithms/NancyDDBackup.h"

using namespace std;

template <class Domain, class Node, class TopLevelAction>
class RiskDD : public ExpansionAlgorithm<Domain, Node, TopLevelAction> {
    typedef typename Domain::State State;
    typedef typename Domain::Cost Cost;
    typedef typename Domain::HashState Hash;

public:
    RiskDD(Domain& domain, double lookahead, int expansionAllocation)
            : domain(domain),
              lookahead(lookahead),
              expansionsPerIteration(expansionAllocation) {}

    void incrementLookahead() { lookahead++; }

    virtual void expand(PriorityQueue<shared_ptr<Node>>& open,
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
            NancyDDBackup<Node, TopLevelAction>::backup2TLA(tlas);

            // risk computation
            int chosenTLAIndex = computeRiskByPSAndGetBestTLA(tlas);

            // Expand under the TLA which holds the lowest risk
            shared_ptr<Node> chosenNode = tlas[chosenTLAIndex]->open_TLA.top();

            //cout << "exp: choseNode \n" << chosenNode->getState() << endl;
			//cout << "exp: choseNode \n";
            //cout << "g " << chosenNode->getGValue() << " h "
                 //<< chosenNode->getHValue() << endl;

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

            // Generate the successors of the chosen node
            vector<State> children = domain.successors(chosenNode->getState());

            // Book keeping for number of nodes generated
            res.nodesGenerated += children.size();

            // Iterate over the successor states
            for (State child : children) {
                // Create a node for this state
                shared_ptr<Node> childNode = make_shared<Node>(
                        chosenNode->getGValue() + domain.getEdgeCost(child),
                        domain.hstart_distribution(child),
                        domain.hstart_distribution_ps(child),
                        domain.heuristic(child),
                        child,
                        chosenNode,
                        chosenNode->getOwningTLA());

                bool dup = duplicateDetection(childNode, closed, open, tlas);

                // Duplicate detection performed
                if (!dup) {
                    // If this state hasn't yet been reached, add
                    // this node open
                    open.push(childNode);
                    closed[child] = childNode;

                    // Add to open of generating TLA
                    tlas[chosenTLAIndex]->open_TLA.push(childNode);
                }
            }
        }
	}

private:
	// In DD this is different, we just use the post search belief
    virtual int computeRiskByPSAndGetBestTLA(vector<shared_ptr<TopLevelAction>>& tlas) {
        int minimalRiskTLA = 0;
        double minimalRisk = numeric_limits<double>::infinity();

        // Start by identifying alpha: the TLA with lowest expected cost
        int alphaTLA = 0;
        double alphaExpectedCost = tlas[0]->getBeliefDD().expectedCost();
        for (int i = 1; i < tlas.size(); i++) {
            if (tlas[i]->getBeliefDD().expectedCost() < alphaExpectedCost) {
                alphaExpectedCost = tlas[i]->getBeliefDD().expectedCost();
                alphaTLA = i;
            }
        }

        // Iterate over the top level actions
        for (int i = 0; i < tlas.size(); i++) {
            // If this TLA has no unique subtree, skip its risk calc, it is
            // pruned
            if (tlas[i]->open_TLA.empty())
                continue;

            // Calculate the risk associated with expanding that node (by using
            // the simulated belief as alpha in risk analysis)
            double riskCalculation = riskAnalysis(alphaTLA, i, tlas);

            // If this is the first TLA risk has been calculated for, it by
            // default minimizes risk...
            // If two actions minimize risk by same value
			// tie break on fhat - > backed-up  f -> g in this order.
            if (riskCalculation == minimalRisk) {
                if (tlas[i]->topLevelNode->getFHatValueFromDist() ==
                        tlas[minimalRiskTLA]
                                ->topLevelNode->getFHatValueFromDist()) {
                    if (tlas[i]->getF_TLA() == tlas[minimalRiskTLA]->getF_TLA()) {
                        if (tlas[i]->topLevelNode->getGValue() ==
                                tlas[minimalRiskTLA]
                                        ->topLevelNode->getGValue()) {
                            if (tlas[i]->topLevelNode->getState().key() >
                                    tlas[minimalRiskTLA]
                                            ->topLevelNode->getState()
                                            .key()) {
                                minimalRiskTLA = i;
                            }
                        } else if (tlas[i]->topLevelNode->getGValue() >
                                tlas[minimalRiskTLA]
                                        ->topLevelNode->getGValue()) {
                            minimalRiskTLA = i;
                        }
                    } else if (tlas[i]->getF_TLA() <
                            tlas[minimalRiskTLA]->getF_TLA()) {
                        minimalRiskTLA = i;
                    }
                } else if (tlas[i]->topLevelNode->getFHatValue() <
                        tlas[minimalRiskTLA]->topLevelNode->getFHatValue()) {
                    minimalRiskTLA = i;
                }
            } else if (riskCalculation < minimalRisk) {
                // Otherwise the TLA with the lower risk replaces the current
                // lowest
                minimalRisk = riskCalculation;
                minimalRiskTLA = i;
            }
        }

        //cout << "min risk value " << minimalRisk << endl;

        return minimalRiskTLA;
    }

    virtual double riskAnalysis(int alphaIndex,
            int simulateTLAIndex,
            const vector<shared_ptr<TopLevelAction>>& tlas) const {
        double risk = 0;

        const auto& alphaBelief = alphaIndex == simulateTLAIndex ?
                tlas[alphaIndex]->getBeliefDD_ps() :
                tlas[alphaIndex]->getBeliefDD();

        const auto alphaGValue = tlas[alphaIndex]->topLevelNode->getGValue();

        // Perform numerical integration to calculate risk associated with
        // taking alpha as the expansion
        for (const auto& alpha : alphaBelief) {
            for (int tla = 0; tla < tlas.size(); tla++) {
                // Don't integrate over alpha as a beta action
                if (tla == alphaIndex)
                    continue;

                const auto& betaBelief = tla == simulateTLAIndex ?
                        tlas[tla]->getBeliefDD_ps() :
                        tlas[tla]->getBeliefDD();

                const auto betaGValue = tlas[tla]->topLevelNode->getGValue();

                // Integrate over values in beta belief
                for (const auto& beta : betaBelief) {
                    // Only use beta costs less than alpha cost
                    // in risk
                    // analysis
                    if (beta.cost < alpha.cost) {
                        // Calculate the risk
                        double value = alpha.probability * beta.probability *
                                (alpha.cost + alphaBelief.getShiftedCost() +
                                               alphaGValue - beta.cost -
                                               betaBelief.getShiftedCost() -
                                               betaGValue);
                        risk += value;
                    } else
						//here Andrew assume, the belief nodes are arranged in ascending order
                        break;
                }
            }
        }

        return risk;
    }

protected:
    Domain& domain;
    double lookahead;
    string sortingFunction;
    int k = 1;
    int expansionsPerIteration;
};
