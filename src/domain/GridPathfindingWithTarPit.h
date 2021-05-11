#pragma once
#include "../utility/PairHash.h"
#include "../utility/SlidingWindow.h"
#include "../utility/debug.h"
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ostream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

class GridPathfindingWithTarPit
{
    using Location = pair<size_t, size_t>;

public:
    typedef double        Cost;
    static constexpr Cost COST_MAX = std::numeric_limits<Cost>::max();

    struct Action
    {
        int moveX;
        int moveY;

        bool isDummy() const { return moveX == 999; }
    };

    class State
    {
    public:
        State() {}

        State(size_t x_, size_t y_, Action a_)
            : x(x_)
            , y(y_)
            , fromAction(a_)
        {
            generateKey();
        }

        friend std::ostream& operator<<(
          std::ostream& stream, const GridPathfindingWithTarPit::State& state)
        {
            stream << "x: " << state.x << " ";
            stream << "y: " << state.y << " ";
            stream << "\n";
            stream << "key " << state.key() << "\n";
            stream << "from action " << state.fromAction.moveX
                   << state.fromAction.moveY << "\n";

            return stream;
        }

        bool operator==(const State& state) const
        {
            return theKey == state.key();
        }

        bool operator!=(const State& state) const
        {
            return theKey != state.key();
        }

        void generateKey()
        {
            // This will provide a unique hash for every state in the
            // vaccumworld, the normal hash_combine seems go out of the search
            // space for 200x200, so we use the slower string combine here
            string sKey = to_string(x) + to_string(y);
            theKey      = std::hash<string>{}(sKey);
        }

        unsigned long long key() const { return theKey; }

        size_t getX() const { return x; }

        size_t getY() const { return y; }

        Action getFromAction() const { return fromAction; }

        std::string toString() const
        {
            return to_string(x) + " " + to_string(y);
        }

        void markStart() { label = 's'; }

        bool isStart() const { return label == 's'; }

    private:
        size_t             x, y;
        char               label;
        Action             fromAction;
        unsigned long long theKey =
          std::numeric_limits<unsigned long long>::max();
    };

    struct HashState
    {
        std::size_t operator()(const State& s) const { return s.key(); }
    };

    GridPathfindingWithTarPit(std::istream& input)
    {
        parseInput(input);
        initilaizeActions();
        // costVariant = 0; // default
        heuristicVariant = 0; // Default
        initialize();
    }

    void setVariant(int variant) { heuristicVariant = variant; }

    // void setVariant(int variant) { costVariant = variant; }

    bool isGoal(const State& s) const
    {
        return s.getX() == goalX && s.getY() == goalY;
    }

    Cost distance(const State& state)
    {

        if (correctedD.find(state) != correctedD.end()) {
            return correctedD[state];
        }

        Cost d = heuristicFunction(state);

        updateDistance(state, d);

        return correctedD[state];
    }

    Cost distanceErr(const State& state)
    {
        // Check if the distance error of this state has been updated
        if (correctedDerr.find(state) != correctedDerr.end()) {
            return correctedDerr[state];
        }

        Cost derr = heuristicFunction(state);

        updateDistanceErr(state, derr);

        return correctedDerr[state];
    }

    Cost heuristic(const State& state)
    {
        // Check if the heuristic of this state has been updated
        if (correctedH.find(state) != correctedH.end()) {
            return correctedH[state];
        }

        Cost h = heuristicFunction(state);

        updateHeuristic(state, h);

        return correctedH[state];
    }

    Cost heuristicFunction(const State& state)
    {
        if (heuristicVariant == 1) {
            return manhattanDistanceToGoal(state);
        }
        return euclideanDistToGoal(state);
    }

    void updateDistance(const State& state, Cost value)
    {
        correctedD[state] = value;
    }

    void updateDistanceErr(const State& state, Cost value)
    {
        correctedDerr[state] = value;
    }

    void updateHeuristic(const State& state, Cost value)
    {
        correctedH[state] = value;
    }

    double getBranchingFactor() const { return 4; }

    bool isLegalLocation(int x, int y) const
    {
        return x >= 0 && y >= 0 && static_cast<size_t>(x) < mapWidth &&
               static_cast<size_t>(y) < mapHeight &&
               blockedCells.find(
                 Location(static_cast<size_t>(x), static_cast<size_t>(y))) ==
                 blockedCells.end();
    }

    std::vector<State> successors(const State& state)
    {
        std::vector<State> successors;

        for (auto action : actions) {

            int newX = static_cast<int>(state.getX()) + action.moveX;
            int newY = static_cast<int>(state.getY()) + action.moveY;

            if (isLegalLocation(newX, newY)) {
                State succ(static_cast<size_t>(newX), static_cast<size_t>(newY),
                           action);
                successors.push_back(succ);
            }
        }

        // recording predecessor
        for (const auto& succ : successors) {
            predecessorsTable[succ].push_back(state);
        }

        return successors;
    }

    const std::vector<State> predecessors(const State& state) const
    {
        // DEBUG_MSG("preds table size: "<<predecessorsTable.size());
        if (predecessorsTable.find(state) != predecessorsTable.end())
            return predecessorsTable.at(state);
        return vector<State>();
    }

    const State getStartState() const { return startState; }

    Cost getEdgeCost(const State& state) const
    {
        if (state.getFromAction().isDummy()) {
            return 0;
        }
        auto parentLoc = getParentLocation(state);
        if (isTarPit(parentLoc)) {
            return tarPitCost;
        }
        return 1;
    }

    string getDomainInformation() const
    {
        string info = "{ \"Domain\": \"grid pathfinding\", \"widthxheight\": " +
                      std::to_string(mapHeight) + "x" +
                      std::to_string(mapHeight) + " }";
        return info;
    }

    string getDomainName() { return "GridPathfindingWithTarPit"; }

    void initialize()
    {
        correctedD.clear();
        correctedH.clear();
        correctedDerr.clear();

        averageExpansionDelay     = 0;
        averageExpansionDelayCntr = 0;
    }

    void pushDelayWindow(unsigned int val)
    {
        ++averageExpansionDelayCntr;

        averageExpansionDelay -=
          averageExpansionDelay / averageExpansionDelayCntr;

        averageExpansionDelay += static_cast<double>(val) /
                                 static_cast<double>(averageExpansionDelayCntr);
    }

    double averageDelayWindow()
    {
        if (averageExpansionDelay < 1)
            return 1;

        return averageExpansionDelay;
    }

    string getSubDomainName() const { return ""; }

private:
    void parseInput(std::istream& input)
    {
        string line;
        getline(input, line);
        stringstream ss(line);
        ss >> mapWidth;

        getline(input, line);

        stringstream ss2(line);
        ss2 >> mapHeight;

        for (size_t y = 0; y < mapHeight; y++) {

            getline(input, line);
            stringstream ss3(line);

            for (size_t x = 0; x < mapWidth; x++) {
                char cell;
                ss3 >> cell;

                switch (cell) {
                    case '#':
                        blockedCells.insert(Location(x, y));
                        break;
                    case '$':
                        tarPitCells.insert(Location(x, y));
                        break;
                    case '*':
                        goalX = x;
                        goalY = y;
                        break;
                    case '@':
                        startLocation = Location(x, y);
                        break;
                }
            }
        }

        getline(input, line); // skip the solution
        getline(input, line);
        stringstream ss4(line);
        ss4 >> tarPitCost;

        cout << "size: " << mapWidth << "x" << mapHeight << "\n";
        cout << "blocked: " << blockedCells.size() << "\n";
        cout << "tarPit: " << tarPitCells.size() << "\n";
        cout << "tarPit Cost: " << tarPitCost << "\n";
        cout << "goalX: " << goalX << "goalY: " << goalY << "\n";
        cout << "startX: " << startLocation.first
             << "startY: " << startLocation.second << "\n";

        startState =
          State(startLocation.first, startLocation.second, Action{999, 999});
    }

    void initilaizeActions()
    {
        // move left, right, up, down
        actions = {Action{-1, 0}, Action{1, 0}, Action{0, -1}, Action{0, 1}};
    }

    double euclideanDistToGoal(const State& s) const
    {
        return sqrt(
          pow(static_cast<int>(s.getX()) - static_cast<int>(goalX), 2.0) +
          pow(static_cast<int>(s.getY()) - static_cast<int>(goalY), 2.0));
    }

    double manhattanDistanceToGoal(const State& s) const
    {
        return static_cast<double>(abs_diff(s.getX(), goalX) +
                                   abs_diff(s.getY(), goalY));
    }

    size_t abs_diff(size_t a, size_t b) const { return a > b ? a - b : b - a; }

    Location getParentLocation(const State& state) const
    {
        return Location(
          static_cast<int>(state.getX()) - state.getFromAction().moveX,
          static_cast<int>(state.getY()) - state.getFromAction().moveY);
    }

    bool isTarPit(const Location& loc) const
    {
        return tarPitCells.find(loc) != tarPitCells.end();
    }

    std::unordered_set<Location, pair_hash> blockedCells;
    std::unordered_set<Location, pair_hash> tarPitCells;
    double                                  tarPitCost;
    vector<Action>                          actions;
    vector<vector<size_t>>                  dijkstraMap;
    size_t                                  mapWidth;
    size_t                                  mapHeight;
    Location                                startLocation;
    State                                   startState;

    double                                         averageExpansionDelay;
    unsigned int                                   averageExpansionDelayCntr;
    unordered_map<State, Cost, HashState>          correctedH;
    unordered_map<State, Cost, HashState>          correctedD;
    unordered_map<State, Cost, HashState>          correctedDerr;
    unordered_map<State, vector<State>, HashState> predecessorsTable;
    int                                            heuristicVariant;

    size_t goalX;
    size_t goalY;
};
