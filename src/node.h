#pragma once
#include <memory>

#include "utility/DiscreteDistribution.h"
#include "utility/debug.h"

using namespace std;

template<class Domain>
class SearchNode
{
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;

    Cost                 g;
    Cost                 h;
    Cost                 d;
    Cost                 derr;
    bool                 open;
    unsigned int         delayCntr;
    DiscreteDistribution distribution;

    double startEpsilonH;
    double startEpsilonD;

    double       curEpsilonH;
    double       curEpsilonD;
    unsigned int expansionCounter;

    shared_ptr<SearchNode<Domain>> parent;
    State                          stateRep;

    shared_ptr<SearchNode<Domain>> nancyFrontier;
    Cost                           backupHHat;

public:
    Cost getGValue() const { return g; }
    Cost getHValue() const { return h; }
    Cost getDValue() const { return d; }
    Cost getDErrValue() const { return derr; }
    Cost getFValue() const { return g + h; }
    Cost getFHatValue() const { return g + getHHatValue(); }
    Cost getDHatValue() const { return (derr / (1.0 - curEpsilonD)); }
    Cost getHHatValue() const { return h + getDHatValue() * curEpsilonH; }
    Cost getBackupHHatValue() const { return backupHHat; }
    shared_ptr<SearchNode<Domain>> getNancyFrontier() const
    {
        return nancyFrontier;
    }

    State                          getState() const { return stateRep; }
    shared_ptr<SearchNode<Domain>> getParent() const { return parent; }

    Cost         getPathBasedEpsilonH() const { return curEpsilonH; }
    Cost         getPathBasedEpsilonD() const { return curEpsilonD; }
    unsigned int getPathBasedExpansionCounter() const
    {
        return expansionCounter;
    }

    void setHValue(Cost val) { h = val; }
    void setGValue(Cost val) { g = val; }
    void setDValue(Cost val) { d = val; }
    void setDErrValue(Cost val) { derr = val; }
    void setEpsilonH(Cost val) { curEpsilonH = val; }
    void setEpsilonD(Cost val) { curEpsilonD = val; }
    void setState(State s) { stateRep = s; }
    void setParent(shared_ptr<SearchNode<Domain>> p) { parent = p; }
    void setBackupHHat(Cost val) { backupHHat = val; }
    void setNancyFrontier(shared_ptr<SearchNode<Domain>> n)
    {
        nancyFrontier = n;
    }

    bool onOpen() { return open; }
    void close() { open = false; }
    void reOpen() { open = true; }

    void markStart() { stateRep.markStart(); }

    void         incDelayCntr() { ++delayCntr; }
    unsigned int getDelayCntr() { return delayCntr; }

    void resetStartEpsilons()
    {
        startEpsilonD = curEpsilonD;
        startEpsilonH = curEpsilonH;
    }

    void pushPathBasedEpsilons(double epsH_, double epsD_)
    {
        incExpansionCounter();
        pushEpsilonHPathBased(epsH_);
        pushEpsilonDPathBased(epsD_);
    }

    void incExpansionCounter() { ++expansionCounter; }

    void pushEpsilonHPathBased(double eps)
    {
        if (expansionCounter < 5) {
            curEpsilonH = startEpsilonH;
            return;
        }

        //DEBUG_MSG("epsh " + to_string(eps));
        //DEBUG_MSG("expCounter " + to_string(expansionCounter));

        curEpsilonH -= curEpsilonH / expansionCounter;
        curEpsilonH += eps / expansionCounter;
    }

    void pushEpsilonDPathBased(double eps)
    {
        if (expansionCounter < 5) {
            curEpsilonH = startEpsilonH;
            return;
        }

        curEpsilonD -= curEpsilonD / expansionCounter;
        curEpsilonD += eps / expansionCounter;
    }

    SearchNode<Domain>(Cost g_, Cost h_, Cost d_, Cost derr_, Cost epsH_,
                       Cost epsD_, unsigned int expansionCounter_, State state_,
                       shared_ptr<SearchNode<Domain>> parent_)
        : g(g_)
        , h(h_)
        , d(d_)
        , derr(derr_)
        , startEpsilonH(epsH_)
        , startEpsilonD(epsD_)
        , expansionCounter(expansionCounter_)
        , parent(parent_)
        , stateRep(state_)
    {
        open      = true;
        delayCntr = 0;
    }

    string toString() const
    {
        string str = "";
        str += "{state: " + stateRep.toString() + ",";
        str += "f: " + my_to_string(getFValue()) + ",";
        str += "g: " + my_to_string(getGValue()) + ",";
        str += "h: " + my_to_string(getHValue()) + ",";
        str += "derr: " + my_to_string(getDErrValue()) + ",";
        str += "d: " + my_to_string(getDValue()) + ",";
        str += "epsilon-h: " + my_to_string(getPathBasedEpsilonH()) + ",";
        str += "epsilon-d: " + my_to_string(getPathBasedEpsilonD()) + ",";
        str += "f-hat: " + my_to_string(getFHatValue()) + ",";
        str += "d-hat: " + my_to_string(getDHatValue()) + ",";
        str += "h-hat: " + my_to_string(getHHatValue()) + "}";
        return str;
    }

    static bool compareNodesF(const shared_ptr<SearchNode<Domain>> n1,
                              const shared_ptr<SearchNode<Domain>> n2)
    {
        // Tie break on g-value
        if (n1->getFValue() == n2->getFValue()) {
            return n1->getGValue() > n2->getGValue();
        }
        return n1->getFValue() < n2->getFValue();
    }

    static bool compareNodesFHat(const shared_ptr<SearchNode<Domain>> n1,
                                 const shared_ptr<SearchNode<Domain>> n2)
    {
        // Tie break on g-value
        if (n1->getFHatValue() == n2->getFHatValue()) {
            if (n1->getFValue() == n2->getFValue()) {
                if (n1->getGValue() == n2->getGValue()) {
                    return n1->getState().key() > n2->getState().key();
                }
                return n1->getGValue() > n2->getGValue();
            }
            return n1->getFValue() < n2->getFValue();
        }
        return n1->getFHatValue() < n2->getFHatValue();
    }

    static bool compareNodesH(const shared_ptr<SearchNode<Domain>> n1,
                              const shared_ptr<SearchNode<Domain>> n2)
    {
        if (n1->getHValue() == n2->getHValue()) {
            return n1->getGValue() > n2->getGValue();
        }
        return n1->getHValue() < n2->getHValue();
    }

    static bool compareNodesHHat(const shared_ptr<SearchNode<Domain>> n1,
                                 const shared_ptr<SearchNode<Domain>> n2)
    {
        if (n1->getHHatValue() == n2->getHHatValue()) {
            return n1->getGValue() > n2->getGValue();
        }
        return n1->getHHatValue() < n2->getHHatValue();
    }

    static bool compareNodesBackedHHat(const shared_ptr<SearchNode<Domain>> n1,
                                       const shared_ptr<SearchNode<Domain>> n2)
    {
        /*if (n1->backupHHat == n2->backupHHat) {*/
        // return n1->getGValue() > n2->getGValue();
        /*}*/
        return n1->backupHHat < n2->backupHHat;
    }

    static double getLowerConfidence(const shared_ptr<SearchNode<Domain>> n)
    {
        double f    = n->getFValue();
        double mean = n->getFHatValue();
        if (f == mean) {
            return f;
        }
        double error  = mean - f;
        double stdDev = error / 2.0;
        double var    = pow(stdDev, 2);
        // 1.96 is the Z value from the Z table to get the 2.5 confidence
        return max(f, mean - (1.96 * var));
    }

    static bool compareNodesLC(const shared_ptr<SearchNode<Domain>> n1,
                               const shared_ptr<SearchNode<Domain>> n2)
    {
        // Lower confidence interval
        if (getLowerConfidence(n1) == getLowerConfidence(n2)) {
            return n1->getGValue() > n2->getGValue();
        }
        return getLowerConfidence(n1) < getLowerConfidence(n2);
    }
};
