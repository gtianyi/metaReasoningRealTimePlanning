#pragma once
#include <iostream>
#include <queue>

using namespace std;

struct ResultContainer {
    queue<string> path;
    bool solutionFound;
    double solutionCost;
    int nodesGenerated;
    int nodesExpanded;
    vector<double> lookaheadCpuTime;
    double epsilonHGlobal;
    double epsilonDGlobal;

    ResultContainer() {
        solutionFound = false;
        solutionCost = -1;
        nodesGenerated = 0;
        nodesExpanded = 0;
        epsilonHGlobal = 0;
        epsilonDGlobal = 0;
    }

    ResultContainer(const ResultContainer& res) {
        solutionFound = res.solutionFound;
        solutionCost = res.solutionCost;
        nodesGenerated = res.nodesGenerated;
        nodesExpanded = res.nodesExpanded;
        path = res.path;
		lookaheadCpuTime = res.lookaheadCpuTime;
		epsilonHGlobal = res.epsilonHGlobal;
		epsilonDGlobal = res.epsilonDGlobal;
    }

    ResultContainer& operator=(const ResultContainer& rhs) {
        if (&rhs == this)
            return *this;
        else {
            solutionFound = rhs.solutionFound;
            solutionCost = rhs.solutionCost;
            nodesGenerated = rhs.nodesGenerated;
            nodesExpanded = rhs.nodesExpanded;
            path = rhs.path;
            lookaheadCpuTime = rhs.lookaheadCpuTime;
            epsilonHGlobal = rhs.epsilonHGlobal;
            epsilonDGlobal = rhs.epsilonDGlobal;

            return *this;
        }
    }
};
