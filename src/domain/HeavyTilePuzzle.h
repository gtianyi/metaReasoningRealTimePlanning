#pragma once
#include "SlidingTilePuzzle.h"

class HeavyTilePuzzle : public SlidingTilePuzzle
{
public:
    using SlidingTilePuzzle::SlidingTilePuzzle;

    Cost getEdgeCost(State state) { return state.getFace(); }

    Cost heuristic(const State& state)
    {
        // Check if the heuristic of this state has been updated
        if (correctedH.find(state) != correctedH.end()) {
            return correctedH[state];
        }

        Cost h = manhattanDistanceWithFaceCost(state);

        updateHeuristic(state, h);

        return correctedH[state];
    }

    Cost manhattanDistanceWithFaceCost(const State& state) const
    {
        Cost manhattanSum = 0;

        for (size_t r = 0; r < size; r++) {
            for (size_t c = 0; c < size; c++) {
                int value = state.getBoard()[r][c];
                if (value == 0) {
                    continue;
                }

                manhattanSum +=
                  static_cast<double>(value) *
                  (fabs(value / static_cast<int>(size) - static_cast<int>(r)) +
                   fabs(value % static_cast<int>(size) - static_cast<int>(c)));
                // cout << "value " << value << " sum " << manhattanSum << endl;
            }
        }

        return manhattanSum;
    }

    string getSubDomainName() const { return "heavy"; }

    Cost distance(const State& state)
    {
        // Check if the distance of this state has been updated
        if (correctedD.find(state) != correctedD.end()) {
            return correctedD[state];
        }

        Cost d = manhattanDistanceWithFaceCost(state);

        updateDistance(state, d);

        return correctedD[state];
    }
};
