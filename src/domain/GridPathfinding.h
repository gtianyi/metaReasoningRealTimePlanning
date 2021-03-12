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

class GridPathfinding
{
    using Location = pair<size_t, size_t>;

public:
    typedef double        Cost;
    static constexpr Cost COST_MAX = std::numeric_limits<Cost>::max();

    struct Action
    {
        int moveX;
        int moveY;
    };

    class State
    {
    public:
        State() {}

        State(size_t x_, size_t y_)
            : x(x_)
            , y(y_)
        {
            generateKey();
        }

        friend std::ostream& operator<<(std::ostream&                 stream,
                                        const GridPathfinding::State& state)
        {
            stream << "x: " << state.x << " ";
            stream << "y: " << state.y << " ";
            stream << "\n";
            stream << "key " << state.key() << "\n";

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

        std::string toString() const
        {
            return to_string(x) + " " + to_string(y) + "\n ";
        }

        void markStart() { label = 's'; }

    private:
        size_t             x, y;
        char               label;
        unsigned long long theKey =
          std::numeric_limits<unsigned long long>::max();
    };

    struct HashState
    {
        std::size_t operator()(const State& s) const { return s.key(); }
    };

    GridPathfinding(std::istream& input)
    {
        parseInput(input);
        initilaizeActions();
        // costVariant = 0; // default
    }

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

        Cost d = euclideanDistToGoal(state);

        updateDistance(state, d);

        return correctedD[state];
    }

    Cost distanceErr(const State& state)
    {
        // Check if the distance error of this state has been updated
        if (correctedDerr.find(state) != correctedDerr.end()) {
            return correctedDerr[state];
        }

        Cost derr = euclideanDistToGoal(state);

        updateDistanceErr(state, derr);

        return correctedDerr[state];
    }

    Cost heuristic(const State& state)
    {
        // Check if the heuristic of this state has been updated
        if (correctedH.find(state) != correctedH.end()) {
            return correctedH[state];
        }

        Cost h = euclideanDistToGoal(state);

        updateHeuristic(state, h);

        return correctedH[state];
    }

    Cost epsilonHGlobal() { return curEpsilonH; }

    Cost epsilonDGlobal() { return curEpsilonD; }

    // Cost epsilonHVarGlobal() { return curEpsilonHVar; }

    void updateEpsilons()
    {
        /*if (expansionCounter < 100) {*/
        // curEpsilonD    = 0;
        // curEpsilonH    = 0;
        // curEpsilonHVar = 0;

        // return;
        /*}*/

        curEpsilonD = epsilonDSum / expansionCounter;

        curEpsilonH = epsilonHSum / expansionCounter;

        /* curEpsilonHVar =*/
        //(epsilonHSumSq - (epsilonHSum * epsilonHSum) / expansionCounter) /
        //(expansionCounter - 1);

        // assert(curEpsilonHVar > 0);
    }

    void pushEpsilonHGlobal(double eps)
    {
        /*if (eps < 0)*/
        // eps = 0;
        // else if (eps > 1)
        /*eps = 1;*/

        epsilonHSum += eps;
        // epsilonHSumSq += eps * eps;
        expansionCounter++;
    }

    void pushEpsilonDGlobal(double eps)
    {
        /*if (eps < 0)*/
        // eps = 0;
        // else if (eps > 1)
        /*eps = 1;*/

        epsilonDSum += eps;
        expansionCounter++;
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
                State succ(static_cast<size_t>(newX),
                           static_cast<size_t>(newY));
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

    Cost getEdgeCost(const State&) const
    {
        // Variants:
        // 0: uniform cost.
        // 1: heavy cost: one plus the number of dirt piles the robot has
        //    cleaned up (the weight from the dirt drains the battery faster).
        /*if (costVariant == 1) {*/
        // if (state.isCleanAction())
        // return state.getCleanedDirtsCount();
        // else
        // return state.getCleanedDirtsCount() + 1;
        /*}*/

        return 1;
    }

    string getDomainInformation() const
    {
        string info = "{ \"Domain\": \"grid pathfinding\", \"widthxheight\": " +
                      std::to_string(mapHeight) + "x" +
                      std::to_string(mapHeight) + " }";
        return info;
    }

    string getDomainName() { return "GridPathfinding"; }

    void initialize(string policy, int la)
    {
        epsilonDSum      = 0;
        epsilonHSum      = 0;
        expansionCounter = 0;
        curEpsilonD      = 0;
        curEpsilonH      = 0;

        expansionPolicy = policy;
        lookahead       = la;
        correctedD.clear();
        correctedH.clear();
        correctedDerr.clear();
        expansionDelayWindow.clear();
    }

    void pushDelayWindow(int val) { expansionDelayWindow.push(val); }

    double averageDelayWindow()
    {
        if (expansionDelayWindow.size() == 0)
            return 1;

        double avg = 0;

        for (auto i : expansionDelayWindow) {
            avg += i;
        }

        avg /= static_cast<double>(expansionDelayWindow.size());

        return avg;
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

        cout << "size: " << mapWidth << "x" << mapHeight << "\n";
        cout << "blocked: " << blockedCells.size() << "\n";
        cout << "goalX: " << goalX << "goalY: " << goalY << "\n";
        cout << "startX: " << startLocation.first
             << "startY: " << startLocation.second << "\n";

        startState = State(startLocation.first, startLocation.second);
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

    std::unordered_set<Location, pair_hash> blockedCells;
    vector<Action>                          actions;
    vector<vector<size_t>>                  dijkstraMap;
    size_t                                  mapWidth;
    size_t                                  mapHeight;
    Location                                startLocation;
    State                                   startState;

    // int costVariant;

    SlidingWindow<int>                             expansionDelayWindow;
    unordered_map<State, Cost, HashState>          correctedH;
    unordered_map<State, Cost, HashState>          correctedD;
    unordered_map<State, Cost, HashState>          correctedDerr;
    unordered_map<State, vector<State>, HashState> predecessorsTable;

    double epsilonHSum;
    // double epsilonHSumSq;
    double epsilonDSum;
    double curEpsilonH;
    double curEpsilonD;
    // double curEpsilonHVar;
    double expansionCounter;

    string expansionPolicy;
    int    lookahead;
    size_t goalX;
    size_t goalY;
};
