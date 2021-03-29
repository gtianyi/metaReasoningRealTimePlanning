#pragma once
#include "../utility/SlidingWindow.h"
#include <algorithm>
#include <bitset>
#include <cmath>
#include <iomanip>
#include <limits>
#include <ostream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class SlidingTilePuzzle
{
public:
    typedef double        Cost;
    static constexpr Cost COST_MAX = std::numeric_limits<Cost>::max();

    class State
    {
    public:
        State() {}

        State(std::vector<std::vector<int>>& b, char l)
            : label(l)
        {
            generateKey(b);
        }

        State(std::vector<std::vector<int>>& b, char l, int f)
            : label(l)
            , movedFace(f)
        {
            generateKey(b);
        }

        friend std::ostream& operator<<(std::ostream&                   stream,
                                        const SlidingTilePuzzle::State& state)
        {
            for (unsigned int r = 0; r < state.getBoard().size(); r++) {
                for (unsigned int c = 0; c < state.getBoard()[r].size(); c++) {
                    stream << std::setw(3) << state.getBoard()[r][c] << " ";
                }
                stream << endl;
            }
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

        unsigned long key() const { return theKey; }

        std::string toString() const
        {

            auto        board = unpack();
            std::string s     = "";
            for (unsigned int r = 0; r < board.size(); r++) {
                for (unsigned int c = 0; c < board[r].size(); c++) {
                    s += std::to_string(board[r][c]) + " ";
                }
                s += "\n";
            }
            return s;
        }

        std::vector<std::vector<int>> getBoard() const
        {
            auto board = unpack();
            return board;
        }

        int getLabel() const { return label; }

        int getFace() const { return movedFace; }

        void markStart() { label = 's'; }

    private:
        void generateKey(const std::vector<std::vector<int>>& board)
        {
            // This will provide a unique hash for every state in the 15 puzzle,
            // Other puzzle variants may/will see collisions...
            unsigned long val = 0;
            for (unsigned int r = 0; r < board.size(); r++) {
                for (unsigned int c = 0; c < board[r].size(); c++) {
                    val = val << 4;
                    val = val | static_cast<unsigned long>(board[r][c]);
                }
            }
            theKey = val;
        }

        std::vector<std::vector<int>> unpack() const
        {
            std::vector<std::vector<int>> board(4, vector<int>(4, 0));

            auto copyOfKey = theKey;

            for (int r = static_cast<int>(board.size()) - 1; r >= 0; r--) {
                for (int c =
                       static_cast<int>(board[static_cast<size_t>(r)].size()) -
                       1;
                     c >= 0; c--) {
                    int t = static_cast<int>(copyOfKey & 0xF);
                    copyOfKey >>= 4;
                    board[static_cast<size_t>(r)][static_cast<size_t>(c)] = t;
                }
            }

            return board;
        }

        char          label;
        int           movedFace;
        unsigned long theKey = std::numeric_limits<unsigned long>::max();
    };

    struct HashState
    {
        std::size_t operator()(const State& s) const
        {
            return s.key();

            /*This tabulation hashing causes mad bugs and non-deterministic
            behavior. Fix later, use shitty hash now and get results...
            unsigned int hash = 0;

            unsigned long long key = s.key();

            // For each byte in the key...
            for (int i = 7; i >= 0; i--)
            {
                    unsigned int byte = key >> (i * 8) & 0x000000FF;
                    hash = leftRotate(hash, 1);
                    hash = hash ^ SlidingTilePuzzle::table[byte];
            }
            cout << key << " " << hash << endl;
            return hash;
            */
        }

        std::size_t leftRotate(std::size_t n, unsigned int d) const
        {
            return (n << d) | (n >> (32 - d));
        }
    };

    SlidingTilePuzzle(std::istream& input)
    {
        // Get the dimensions of the puzzle
        string line;
        getline(input, line);
        stringstream ss(line);
        // Get the first dimension...
        ss >> size;
        // We don't give a shit about the second dimension,
        // because every puzzle should be square.

        // Skip the next line
        getline(input, line);

        // Initialize the nxn puzzle board
        std::vector<int>              rows(static_cast<size_t>(size), 0);
        std::vector<std::vector<int>> board(static_cast<size_t>(size), rows);
        startBoard = board;
        endBoard   = board;

        // Following lines are the input puzzle...
        size_t r = 0;
        size_t c = 0;

        for (size_t i = 0; i < size * size; i++) {
            c = i % size;

            getline(input, line);
            int          tile;
            stringstream ss2(line);
            ss2 >> tile;

            startBoard[r][c] = tile;

            if (c >= size - 1) {
                r++;
            }
        }

        // Skip the next line
        getline(input, line);

        // Following lines are the goal puzzle...
        r = 0;
        c = 0;

        for (size_t i = 0; i < size * size; i++) {
            c = i % size;

            getline(input, line);
            int          tile;
            stringstream ss2(line);
            ss2 >> tile;

            endBoard[r][c] = tile;

            if (c >= size - 1) {
                r++;
            }
        }

        // If the table of random numbers for the hash function hasn't been
        // filled
        // then it should be filled now...
        if (SlidingTilePuzzle::table.empty()) {
            srand(static_cast<unsigned int>(time(NULL)));
            for (int i = 0; i < 256; i++) {
                table.push_back(rand());
            }
        }

        startState = State(startBoard, 's');
        initialize();
    }

    virtual ~SlidingTilePuzzle() {}

    bool isGoal(const State& s) const
    {
        if (s.getBoard() == endBoard) {
            return true;
        }

        return false;
    }

    virtual Cost distance(const State& state)
    {
        // Check if the distance of this state has been updated
        if (correctedD.find(state) != correctedD.end()) {
            return correctedD[state];
        }

        Cost d = manhattanDistance(state);

        updateDistance(state, d);

        return correctedD[state];
    }

    Cost distanceErr(const State& state)
    {
        // Check if the distance error of this state has been updated
        if (correctedDerr.find(state) != correctedDerr.end()) {
            return correctedDerr[state];
        }

        Cost derr = manhattanDistance(state);

        updateDistanceErr(state, derr);

        return correctedDerr[state];
    }

    virtual Cost heuristic(const State& state)
    {
        // Check if the heuristic of this state has been updated
        if (correctedH.find(state) != correctedH.end()) {
            return correctedH[state];
        }

        Cost h = manhattanDistance(state);

        updateHeuristic(state, h);

        return correctedH[state];
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

    Cost manhattanDistance(const State& state) const
    {
        Cost manhattanSum = 0;

        for (size_t r = 0; r < size; r++) {
            for (size_t c = 0; c < size; c++) {
                int value = state.getBoard()[r][c];
                if (value == 0) {
                    continue;
                }

                manhattanSum +=
                  fabs(value / static_cast<int>(size) - static_cast<int>(r)) +
                  fabs(value % static_cast<int>(size) - static_cast<int>(c));
                // cout << value << " sum " << manhattanSum << endl;
            }
        }

        return manhattanSum;
    }

    double getBranchingFactor() const { return 2.13; }

    void moveUp(std::vector<State>&           succs,
                std::vector<std::vector<int>> board) const
    {
        size_t r = 0;
        size_t c = 0;
        // Find the location of the blank space
        bool found = false;
        for (r = 0; r < size; r++) {
            for (c = 0; c < size; c++) {
                if (board[r][c] == 0) {
                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }

        // Now try to move the blank tile up one row...
        if (r > 0) {
            std::swap(board[r][c], board[r - 1][c]);
            succs.push_back(State(board, 'U', board[r][c]));
        }
    }

    void moveDown(std::vector<State>&           succs,
                  std::vector<std::vector<int>> board) const
    {
        size_t r = 0;
        size_t c = 0;
        // Find the location of the blank space
        bool found = false;
        for (r = 0; r < size; r++) {
            for (c = 0; c < size; c++) {
                if (board[r][c] == 0) {
                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }

        // Now try to move the blank tile down one row...
        if (r + 1 < size) {
            std::swap(board[r][c], board[r + 1][c]);
            succs.push_back(State(board, 'D', board[r][c]));
        }
    }

    void moveLeft(std::vector<State>&           succs,
                  std::vector<std::vector<int>> board) const
    {
        size_t r = 0;
        size_t c = 0;
        // Find the location of the blank space
        bool found = false;
        for (r = 0; r < size; r++) {
            for (c = 0; c < size; c++) {
                if (board[r][c] == 0) {
                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }

        // Now try to move the blank tile left one column...
        if (c > 0) {
            std::swap(board[r][c], board[r][c - 1]);
            succs.push_back(State(board, 'L', board[r][c]));
        }
    }

    void moveRight(std::vector<State>&           succs,
                   std::vector<std::vector<int>> board) const
    {
        size_t r = 0;
        size_t c = 0;
        // Find the location of the blank space
        bool found = false;
        for (r = 0; r < size; r++) {
            for (c = 0; c < size; c++) {
                if (board[r][c] == 0) {
                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }

        // Now try to move the blank tile left one column...
        if (c + 1 < size) {
            std::swap(board[r][c], board[r][c + 1]);
            succs.push_back(State(board, 'R', board[r][c]));
        }
    }

    std::vector<State> successors(const State& state) const
    {
        std::vector<State> successors;

        // Don't allow inverse actions, to cut down on branching factor

        if (state.getLabel() != 'D')
            moveUp(successors, state.getBoard());
        if (state.getLabel() != 'U')
            moveDown(successors, state.getBoard());
        if (state.getLabel() != 'R')
            moveLeft(successors, state.getBoard());
        if (state.getLabel() != 'L')
            moveRight(successors, state.getBoard());

        return successors;
    }

    std::vector<State> predecessors(const State& state) const
    {
        std::vector<State> predecessors;

        moveUp(predecessors, state.getBoard());
        moveDown(predecessors, state.getBoard());
        moveLeft(predecessors, state.getBoard());
        moveRight(predecessors, state.getBoard());

        return predecessors;
    }

    bool safetyPredicate(const State&) const { return true; }

    const State getStartState() const { return startState; }

    virtual Cost getEdgeCost(State) { return 1; }

    string getDomainInformation()
    {
        string info =
          "{ \"Domain\": \"Sliding Tile Puzzle\", \"Dimensions\": " +
          std::to_string(size) + "x" + std::to_string(size) + " }";
        return info;
    }

    string getDomainName() { return "SlidingTilePuzzle"; }

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

    bool validatePath(queue<char> path)
    {
        std::vector<std::vector<int>> board = startBoard;

        std::vector<State> successors;

        while (!path.empty()) {
            char action = path.front();
            path.pop();
            if (action == 'U') {
                moveUp(successors, board);
                board = successors.back().getBoard();
            } else if (action == 'D') {
                moveDown(successors, board);
                board = successors.back().getBoard();
            } else if (action == 'R') {
                moveRight(successors, board);
                board = successors.back().getBoard();
            } else if (action == 'L') {
                moveLeft(successors, board);
                board = successors.back().getBoard();
            }
        }

        if (board == endBoard)
            return true;
        return false;
    }

    virtual string getSubDomainName() const { return "uniform"; }

    std::vector<std::vector<int>>         startBoard;
    std::vector<std::vector<int>>         endBoard;
    size_t                                size;
    State                                 startState;
    double                                averageExpansionDelay;
    unsigned int                          averageExpansionDelayCntr;
    unordered_map<State, Cost, HashState> correctedH;
    unordered_map<State, Cost, HashState> correctedD;
    unordered_map<State, Cost, HashState> correctedDerr;

    static vector<int> table;
};

vector<int> SlidingTilePuzzle::table;
