#pragma once
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <memory>

using namespace std;

class DiscreteDistributionDD {
public:
    struct ProbabilityNode {
        double cost;
        double probability;
        ProbabilityNode() = delete;
        ProbabilityNode(double x, double prob) : cost(x), probability(prob) {}
    };

private:
    shared_ptr<vector<ProbabilityNode>> distribution_origin;
    double original_mean;
    double shifted_cost;

    static unordered_map<int, shared_ptr<vector<ProbabilityNode>>> hValueTable;
    static unordered_map<int, shared_ptr<vector<ProbabilityNode>>> hPostSearchTable;

	void copy( const DiscreteDistributionDD& rhs) {
        distribution_origin = rhs.distribution_origin;
		original_mean = rhs.original_mean;
		shifted_cost = rhs.shifted_cost;
    }

public:
    DiscreteDistributionDD() {}

    DiscreteDistributionDD(double h, bool isPostSearch = false) {
        // hash table key is int
        //int hIndex = int(round(h * 100)); //have to fix this for inverse
        int hIndex = int(h);

        //cout << "see a new state, h: " << hIndex << endl;

        unordered_map<int,
                shared_ptr<vector<DiscreteDistributionDD::ProbabilityNode>>>&
                table = isPostSearch ? hPostSearchTable : hValueTable;

        bool isShift = false;
        int deltaH = 1;

        if (table.find(hIndex) == table.end()) {
            cout << "not found h index" << hIndex << endl;

            while (table.find(hIndex - deltaH) == table.end())
                deltaH++;

            cout << "shift from h index" << hIndex - deltaH << endl;

            hIndex -= deltaH;

            isShift = true;
        }

        distribution_origin = table[hIndex];

        for (const auto& pnode : *distribution_origin) {
            original_mean += pnode.cost * pnode.probability;
        }

        if (isShift)
            shifted_cost = deltaH;
    }

    DiscreteDistributionDD(const DiscreteDistributionDD& rhs) {
        if (&rhs == this) {
            return ;
        }
		copy(rhs);
	}

    // create a distribution by shifting from the predescssor distribution
    DiscreteDistributionDD(const DiscreteDistributionDD& rhs,
            double shiftCost) {
        if (&rhs == this) {
            return ;
        }
		copy(rhs);
		shifted_cost+=shiftCost;
    }

    double expectedCost() const { return original_mean + shifted_cost; }

    DiscreteDistributionDD& operator=(const DiscreteDistributionDD& rhs) {
        if (&rhs == this) {
            return *this;
        }

		copy(rhs);

        return *this;
    }

    template <class Domain>
    static void readData(shared_ptr<Domain> domain) {

        string fileName = domain->getDistributionFile();
        string fileName_ps = domain->getDistributionFile_ps();

        ifstream f(fileName);
        ifstream f_ps(fileName_ps);

        if (!f.good() || !f_ps.good()) {
            cout << "No distribution data file!\n";
            exit(1);
		}

		cout << "read d\n";
		domain->readDistributionData(f, hValueTable);

		cout << "read postd\n";
		domain->readDistributionData(f_ps, hPostSearchTable);
    }

    vector<ProbabilityNode>::iterator begin() const {
        return distribution_origin->begin();
    }

    vector<ProbabilityNode>::iterator end() const { return distribution_origin->end(); }

    double getShiftedCost() const { return shifted_cost; }

	//this is for testing the squish assumption
	DiscreteDistributionDD squish(double factor) {
		DiscreteDistributionDD ret;
		ret.original_mean = original_mean;
		ret.shifted_cost = shifted_cost;
		ret.distribution_origin = make_shared<vector<ProbabilityNode>>();
		auto& newDistribution = ret.distribution_origin;

        // If the squish factor is 1, all values in distribution will be moved
        // to the mean.
        if (factor == 1) {
			newDistribution->push_back(ProbabilityNode(original_mean, 1.0));
            return ret;
        }

        for (const auto& n : *distribution_origin) {
            double distanceToMean = abs(n.cost - original_mean);
            double distanceToShift = distanceToMean * factor;

            double shiftedCost = n.cost;

            if (shiftedCost > original_mean)
                shiftedCost -= distanceToShift;
            else if (shiftedCost < original_mean)
                shiftedCost += distanceToShift;

            newDistribution->push_back(ProbabilityNode(shiftedCost, n.probability));
        }

        return ret;
    }
};

unordered_map<int,
        std::shared_ptr<vector<DiscreteDistributionDD::ProbabilityNode>>>
        DiscreteDistributionDD::hValueTable;
unordered_map<int,
        std::shared_ptr<vector<DiscreteDistributionDD::ProbabilityNode>>>
        DiscreteDistributionDD::hPostSearchTable;
