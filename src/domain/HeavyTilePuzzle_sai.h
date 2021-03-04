#pragma once
#include "SlidingTilePuzzle_sai.h"

class HeavyTilePuzzle : public SimpleTilePuzzle {
public:
    using SimpleTilePuzzle::SimpleTilePuzzle;

    Cost getEdgeCost(State state) { return state.getFace(); }

	Cost heuristic(const State& state) {
        // Check if the heuristic of this state has been updated
        if (correctedH.find(state) != correctedH.end()) {
            return correctedH[state];
        }

        Cost h = manhattanDistanceWithFaceCost(state);

        updateHeuristic(state, h);

        return correctedH[state];
    }

    Cost manhattanDistanceWithFaceCost(const State& state) const {
        Cost manhattanSum = 0;

        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                auto value = state.getBoard()[r][c];
                if (value == 0) {
                    continue;
                }

                manhattanSum +=
                        value * (abs(value / size - r) + abs(value % size - c));
				//cout << "value " << value << " sum " << manhattanSum << endl;
            }
        }

        return manhattanSum;
    }
};
