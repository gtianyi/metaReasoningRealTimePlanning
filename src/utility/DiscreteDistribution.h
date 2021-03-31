#pragma once
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace std;

#define M_PI 3.14159265358979323846 /* pi */

class DiscreteDistribution
{
public:
    struct ProbabilityNode
    {
        double cost;
        double probability;

        ProbabilityNode() {}

        ProbabilityNode(double x_, double prob_)
            : cost(x_)
            , probability(prob_)
        {}

        bool operator<(const ProbabilityNode& node) const
        {
            return this->cost < node.cost;
        }

        bool operator>(const ProbabilityNode& node) const
        {
            return this->cost > node.cost;
        }

        bool operator==(const ProbabilityNode& node) const
        {
            return (this->cost == node.cost) &&
                   (this->probability == node.probability);
        }

        bool operator!=(const ProbabilityNode& node) const
        {
            return !(*this == node);
        }

        void shift(double shiftCost) { cost += shiftCost; }
    };

    ~DiscreteDistribution() {}

private:
    struct ProbabilityPair
    {
        ProbabilityNode  first;
        ProbabilityNode  second;
        ProbabilityPair* left  = NULL;
        ProbabilityPair* right = NULL;

        ProbabilityPair(ProbabilityNode lower_, ProbabilityNode upper_)
            : first(lower_)
            , second(upper_)
            , left(NULL)
            , right(NULL)
        {}
    };

    struct CompareDistance
    {
        bool operator()(ProbabilityPair* p1, ProbabilityPair* p2)
        {
            return (p1->second.cost - p1->first.cost) >
                   (p2->second.cost - p2->first.cost);
        }
    };

    set<ProbabilityNode> distribution;
    size_t               maxSamples;
    double               var;
    double               mean;

    double probabilityDensityFunction(double x, double mu, double var_)
    {
        return ((1 / sqrt(2 * M_PI * var_)) *
                exp(-(pow(x - mu, 2) / (2 * var_))));
    }

    void resize(map<double, double>& distroMap)
    {
        // Maybe we don't need to merge any buckets...
        if (distroMap.size() <= maxSamples) {
            return;
        }

        // Gotta merge some buckets...
        priority_queue<ProbabilityPair*, vector<ProbabilityPair*>,
                       CompareDistance>
          heap;

        // Groups probabilities into adjacent pairs and does some pointer
        // assignment for tracking merges
        int              cnt = 0;
        ProbabilityNode  lastNode;
        ProbabilityPair* lastPair = NULL;
        for (map<double, double>::iterator it = distroMap.begin();
             it != distroMap.end(); it++) {
            ProbabilityNode n(it->first, it->second);

            if (cnt == 0) {
                cnt++;
                lastNode = n;
                continue;
            }

            ProbabilityPair* p = new ProbabilityPair(lastNode, n);
            heap.push(p);

            p->left  = lastPair;
            p->right = NULL;
            if (lastPair)
                lastPair->right = p;
            lastPair = p;
            lastNode = n;
            cnt++;
        }

        // Now, while we still have too many samples, and the heap isn't empty,
        // merge buckets
        while (distroMap.size() > maxSamples && !heap.empty()) {
            // Get the pair with the lowest distance between buckets
            ProbabilityPair* merge = heap.top();
            heap.pop();

            // Calculate the new probability and X of the merged bucket
            double newProb =
              merge->first.probability + merge->second.probability;
            double newX =
              (merge->first.probability / newProb) * merge->first.cost +
              (merge->second.probability / newProb) * merge->second.cost;

            ProbabilityNode newNode(newX, newProb);

            // Either add this probability to the existing bucket or make a new
            // bucket for it
            distroMap[newX] += newProb;

            // Remove the old probabilities
            distroMap.erase(merge->first.cost);
            distroMap.erase(merge->second.cost);

            // If merge has a pair on its left, update it
            if (merge->left) {
                merge->left->second = newNode;
                merge->left->right  = merge->right;
            }
            // If merge has a pair on its right, update it
            if (merge->right) {
                merge->right->first = newNode;
                merge->right->left  = merge->left;
            }

            // Delete the merged pair
            delete merge;
        }

        // Delete everything on the heap
        while (!heap.empty()) {
            ProbabilityPair* p = heap.top();
            heap.pop();
            delete p;
        }

        // If we still have too many samples, do it again
        if (distroMap.size() > maxSamples)
            resize(distroMap);
    }

public:
    DiscreteDistribution() {}
    DiscreteDistribution(size_t maxSamples_)
        : maxSamples(maxSamples_)
    {}

    // create from mean and variance
    DiscreteDistribution(size_t maxSamples_, double mean_, double var_)
        : maxSamples(maxSamples_)
        , var(var_)
        , mean(mean_)
    {
        // This is a goal node, belief is a spike at true value
        if (var == 0) {
            distribution.insert(ProbabilityNode(mean, 1.0));
            return;
        }

        // Create a Discrete Distribution from a gaussian
        double lower = max(0.0, mean - 3 * sqrt(var_));
        double upper = mean + 3 * sqrt(var_);

        double sampleStepSize =
          static_cast<double>(upper - lower) / static_cast<double>(maxSamples);

        double currentX = lower;

        double probSum = 0.0;

        vector<ProbabilityNode> tmp;

        // Take the samples and build the discrete distribution
        for (size_t i = 0; i < maxSamples; i++) {
            // Get the probability for this x value
            double prob = probabilityDensityFunction(currentX, mean, var);

            // So if this a goal node, we know the cost
            if (std::isnan(prob) && var == 0)
                prob = 1.0;

            probSum += prob;

            ProbabilityNode node(currentX, prob);

            tmp.push_back(node);

            currentX += sampleStepSize;
        }

        // Normalize the distribution probabilities
        for (ProbabilityNode& n : tmp) {
            if (probSum > 0.0 && n.probability != 1.0)
                n.probability = n.probability / probSum;
            distribution.insert(n);
        }
    }

    double expectedCost() const { return mean; }

    DiscreteDistribution& operator=(const DiscreteDistribution& rhs)
    {
        if (&rhs == this) {
            return *this;
        }

        // distribution.clear();

        distribution = rhs.distribution;
        maxSamples   = rhs.maxSamples;
        mean         = rhs.mean;

        return *this;
    }

    DiscreteDistribution operator*(const DiscreteDistribution& rhs)
    {
        DiscreteDistribution csernaDistro(min(maxSamples, rhs.maxSamples));

        map<double, double> results;

        for (ProbabilityNode n1 : distribution) {
            for (ProbabilityNode n2 : rhs.distribution) {
                double probability = (n1.probability * n2.probability);

                // Don't add to the distribution if the probability of this cost
                // is 0
                if (probability > 0)
                    results[min(n1.cost, n2.cost)] += probability;
            }
        }

        csernaDistro.resize(results);

        for (map<double, double>::iterator it = results.begin();
             it != results.end(); it++) {
            csernaDistro.distribution.insert(
              ProbabilityNode(it->first, it->second));
        }

        return csernaDistro;
    }

    set<ProbabilityNode>::iterator begin() const
    {
        return distribution.begin();
    }

    set<ProbabilityNode>::iterator end() const { return distribution.end(); }

    size_t getDistSize() const { return distribution.size(); }

    DiscreteDistribution(const DiscreteDistribution& rhs)
    {
        if (&rhs == this) {
            return;
        }

        // distribution.clear();

        distribution = rhs.distribution;
        maxSamples   = rhs.maxSamples;
        mean         = rhs.mean;
    }
};
