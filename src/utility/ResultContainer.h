#pragma once
#include <iostream>
#include <queue>

using namespace std;

struct ResultContainer {
    queue<string> path;
    bool solutionFound;
    double solutionCost;
    double solutionLength;
    size_t nodesGenerated;
    size_t GATnodesExpanded;
    size_t nodesExpanded;
    vector<double> lookaheadCpuTime;
    double epsilonHGlobal;
    double epsilonDGlobal;

    ResultContainer() {
        solutionFound = false;
        solutionCost = -1;
        solutionLength = -1;
        nodesGenerated = 0;
        nodesExpanded = 0;
        GATnodesExpanded = 0;
        epsilonHGlobal = 0;
        epsilonDGlobal = 0;
    }

    ResultContainer(const ResultContainer& res) {
        solutionFound = res.solutionFound;
        solutionCost = res.solutionCost;
        solutionLength = res.solutionLength;
        nodesGenerated = res.nodesGenerated;
        nodesExpanded = res.nodesExpanded;
        GATnodesExpanded = res.GATnodesExpanded;
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
            solutionLength = rhs.solutionLength;
            nodesGenerated = rhs.nodesGenerated;
            nodesExpanded = rhs.nodesExpanded;
            GATnodesExpanded = rhs.GATnodesExpanded;
            path = rhs.path;
            lookaheadCpuTime = rhs.lookaheadCpuTime;
            epsilonHGlobal = rhs.epsilonHGlobal;
            epsilonDGlobal = rhs.epsilonDGlobal;

            return *this;
        }
    }
};
