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

#define M_PI 3.14159265358979323846     /* pi */
#define INV_SQRT_2PI 0.3989422804014327 /* inv_sqrt_2pi */

double standardNormalPDF(double x)
{
    return INV_SQRT_2PI * std::exp(-0.5 * x * x);
}

double standardNormalCDF(double x)
{
    return 0.5 * erfc(-x * M_SQRT1_2);
}

double cumulative_distribution(double x)
{
    return (1 + std::erf(x / std::sqrt(2.))) / 2.;
}

double normalPDF(double x, double m, double s)
{
    double a = (x - m) / s;
    return INV_SQRT_2PI / s * std::exp(-0.5 * a * a);
}

class NormalDistribution
{
private:
    double var;
    double mean;

public:
    NormalDistribution(double mean_, double var_)
        : var(var_)
        , mean(mean_)
    {}

    double getMean() const { return mean; }
    double getVar() const { return var; }

    NormalDistribution& operator=(const NormalDistribution& rhs)
    {
        if (&rhs == this) {
            return *this;
        }

        mean = rhs.mean;
        var  = rhs.var;

        return *this;
    }

    NormalDistribution operator*(const NormalDistribution& rhs)
    {
        // TODO
        cerr << "TODO: not implement!";
        exit(1);
        return rhs;
    }

    NormalDistribution(const NormalDistribution& rhs)
    {
        if (&rhs == this) {
            return;
        }

        mean = rhs.mean;
        var  = rhs.var;
    }
};

double computeProbOfd1IsLowerCost(const NormalDistribution& d1,
                                  const NormalDistribution& d2)
{
    if (d1.getVar() == 0 && d2.getVar() == 0) {
        auto prob = d1.getMean() <= d2.getMean() ? 1. : 0.;
        return prob;
    }

    if (d1.getVar() == 0) {
        return cumulative_distribution((d2.getMean() - d1.getMean()) /
                                       sqrt(d2.getVar()));
    }

    auto mean               = d1.getMean() - d2.getMean();
    auto variance           = d1.getVar() + d2.getVar();
    auto standard_deviation = sqrt(variance);

    // compute P(X>0)
    auto prob = cumulative_distribution((mean - 0) / standard_deviation);

    return prob;
}
