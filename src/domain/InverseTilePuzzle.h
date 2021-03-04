#pragma once
#include "SlidingTilePuzzle.h"
#include <math.h>
#include <algorithm>
#include <random>

class InverseTilePuzzle : public SlidingTilePuzzle {
    using pNode = DiscreteDistributionDD::ProbabilityNode;
    using HDistributionMap = unordered_map<int, shared_ptr<vector<pNode>>>;
    using HData = unordered_map<int, vector<double>>;

private:
    template <class T>
    vector<T> getSampleByPercent(const double percent, vector<T> in) const {
        vector<T> out;

        int n = (int)((double)in.size() * percent);
        n = max(n, 1);

        //cout << "numberofSample " << n << endl;

        while (n > 0) {
            std::random_device rd;
            std::mt19937 eng(rd());
            std::uniform_int_distribution<> distr(0, in.size() - 1);

            int randPos = distr(eng);

            out.push_back(in[randPos]);
            in.erase(std::next(in.begin(), randPos));

            n--;
        }
        return out;
    }

    vector<double> getApproximateSortedHsList_forPoint1(const int h, const HData& hvshsData) const {
        int floorh = floor(h / 10.0) * 10; // 113 -> 110 (1.13 -> 1.1)
        int ceilh = ceil(h / 10.0) * 10;// 113 -> 120 (1.13 -> 1.2)
        //cout << "test floorh" << floorh << "ceilh" << ceilh << endl;

        while (hvshsData.find(floorh) == hvshsData.end() && floorh > 0)
            floorh -= 10;

        while (hvshsData.find(ceilh) == hvshsData.end() && ceilh < 1180)
            ceilh += 10;

        //cout << "nearest available floorh" << floorh << "nearest available ceilh" << ceilh << endl;

        const vector<double>& floorHs = hvshsData.at(floorh);
        const vector<double>& ceilHs = hvshsData.at(ceilh);

        if (floorh == ceilh)
            return floorHs;

		//get more sample if you are closer to that bin
        double percent = (double)(ceilh - h) / (double)(ceilh - floorh);
        vector<double> sampleFromFloor =
                getSampleByPercent<double>(percent, floorHs);

        percent = (double)(h - floorh) / (double)(ceilh - floorh);
        vector<double> sampleFromCeil =
                getSampleByPercent<double>(percent, ceilHs);

        vector<double> ret = sampleFromFloor;
        ret.insert(ret.end(), sampleFromCeil.begin(), sampleFromCeil.end());
        sort(ret.begin(), ret.end());

        return ret;
    }

    vector<double> getApproximateSortedHsList(const int h,
            const HData& hvshsData) const {

		if(hvshsData.find(h)!=hvshsData.end())
		{
				return hvshsData.at(h);
		}

		int lefth = h;
		int righth = h;

        while (hvshsData.find(lefth) == hvshsData.end() && lefth > 0)
            lefth -= 1;

        while (hvshsData.find(righth) == hvshsData.end() && righth < 118)
            righth += 1;

        // cout << "nearest available floorh" << floorh << "nearest available
        // ceilh" << ceilh << endl;

        const vector<double>& leftHs = hvshsData.at(lefth);
        const vector<double>& rightHs = hvshsData.at(righth);

        // get more sample if you are closer to that bin
        double percent = (double)(righth - h) / (double)(righth - lefth);
        vector<double> sampleFromFloor =
                getSampleByPercent<double>(percent, leftHs);

        percent = (double)(h - lefth) / (double)(righth - lefth);
        vector<double> sampleFromCeil =
                getSampleByPercent<double>(percent, rightHs);

        vector<double> ret = sampleFromFloor;
        ret.insert(ret.end(), sampleFromCeil.begin(), sampleFromCeil.end());
        sort(ret.begin(), ret.end());

        return ret;
   }

   shared_ptr<vector<pNode>> getDistributionBySortedhsList(
           const vector<double>& sortedhsList) const {
       shared_ptr<vector<pNode>> ret = make_shared<vector<pNode>>();
       unordered_map<double, double> frequency;

       for (const auto& hs : sortedhsList) {
           if (frequency.find(hs) != frequency.end()) {
               continue;
           }
           frequency[hs] =
                   std::count(sortedhsList.begin(), sortedhsList.end(), hs) /
                   (double)sortedhsList.size();
       }

       for (const auto& i : frequency) {
           DiscreteDistributionDD::ProbabilityNode pn(i.first, i.second);
           ret->push_back(pn);
       }

       return ret;
    }

    void approximateHtableByData(const HData& hvshsData,
            HDistributionMap& hValueTable,
            const int maxH) const {
        int resolution = 1;
        int  oneBucket = 1;  
        int step = oneBucket / resolution; // 1 so that hash table key could be int

        for (int h = 0; h <= maxH; h += step) {
            // might have to resolve unseen data here.
            vector<double> sampledSortedHsList =
                    getApproximateSortedHsList(h, hvshsData);

            shared_ptr<vector<pNode>> distribution =
                    getDistributionBySortedhsList(sampledSortedHsList);
            hValueTable.insert({h, distribution});
        }
   }

public:
    using SlidingTilePuzzle::SlidingTilePuzzle;

    Cost getEdgeCost(State state) { return 1.0 / (double)state.getFace(); }

    Cost heuristic(const State& state) {
        // Check if the heuristic of this state has been updated
        if (correctedH.find(state) != correctedH.end()) {
            return correctedH[state];
        }

        Cost h = manhattanDistanceWithInverseFaceCost(state);

        updateHeuristic(state, h);

        return correctedH[state];
    }

    DiscreteDistributionDD hstart_distribution(const State& state) {
        // Check if the heuristic h-hat of this state has been updated
        if (correctedDistribution.find(state) != correctedDistribution.end()) {
            return correctedDistribution[state];
        }

        Cost h = manhattanDistanceWithInverseFaceCost(state) * 10;

        correctedDistribution[state] = DiscreteDistributionDD(h);
        correctedPostSearchDistribution[state] = DiscreteDistributionDD(h,true);

        updateHeuristic(state, h);

        return correctedDistribution[state];
    }

    DiscreteDistributionDD hstart_distribution_ps(const State& state) {
        // Check if the heuristic h-hat of this state has been updated
        if (correctedDistribution.find(state) != correctedDistribution.end()) {
            return correctedDistribution[state];
        }

        Cost h = manhattanDistanceWithInverseFaceCost(state) * 10;

        correctedDistribution[state] = DiscreteDistributionDD(h);
        correctedPostSearchDistribution[state] = DiscreteDistributionDD(h,true);

        return correctedPostSearchDistribution[state];
    }


    Cost manhattanDistanceWithInverseFaceCost(const State& state) const {
        Cost manhattanSum = 0;

        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                auto value = state.getBoard()[r][c];
                if (value == 0) {
                    continue;
                }

                manhattanSum += (1.0 / (double)value) *
                        (abs(value / size - r) + abs(value % size - c));
                // cout << "value " << value << " sum " << manhattanSum << endl;
            }
        }

        return manhattanSum;
    }

    void readDistributionData(ifstream& f,
            HDistributionMap& hValueTable) const {
        //cout << "reading inverse tile data\n";
        string line;

        double h, valueCount, hs, hsCount;
        int maxH = 0;
        int intH = 0;

        HData hData;

        while (getline(f, line)) {
            stringstream ss(line);

            ss >> h;
            intH = int(round(h * 10));
            maxH = max(maxH, intH);

            ss >> valueCount;

            if (hData.find(intH) != hData.end()) {
                cout << "error: duplicate h from data " << h << "\n";
            }

            vector<double> hsList;
            while (!ss.eof()) {
                ss >> hs;
                ss >> hsCount;

                while (hsCount > 0) {
                    hsCount--;
					hsList.push_back(hs);
                }
            }
            hData[intH] = hsList;
        }

		f.close();

		approximateHtableByData(hData, hValueTable, maxH);

		//cout << "total h " << hValueTable.size() << "\n";
    }

    string getSubDomainName() const { return "inverse"; }

    string getDistributionFile() const {
        return "/home/aifs1/gu/phd/research/workingPaper/realtime-nancy/"
               "results/SlidingTilePuzzle/sampleData/"
               "inverse-wastar-statSummary-d.json"; }

    string getDistributionFile_ps() const {
        return "/home/aifs1/gu/phd/research/workingPaper/realtime-nancy/"
               "results/SlidingTilePuzzle/sampleData/"
               "inverse-wastar-statSummary-postd.json"; }

};
