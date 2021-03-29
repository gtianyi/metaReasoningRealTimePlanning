#pragma once
#include <memory>

#include "utility/DiscreteDistribution.h"

using namespace std;

template<class Domain>
class Node
{
    typedef typename Domain::State     State;
    typedef typename Domain::Cost      Cost;
    typedef typename Domain::HashState Hash;

    Cost                 g;
    Cost                 h;
    Cost                 d;
    Cost                 derr;
    shared_ptr<Node>     parent;
    State                stateRep;
    bool                 open;
    unsigned int         delayCntr;
    DiscreteDistribution distribution;

    double       curEpsilonH;
    double       curEpsilonD;
    unsigned int expansionCounter;

    shared_ptr<Node> nancyFrontier;
    Cost             backupHHat;

public:
    Cost getGValue() const { return g; }
    Cost getHValue() const { return h; }
    Cost getDValue() const { return d; }
    Cost getDErrValue() const { return derr; }
    Cost getFValue() const { return g + h; }
    Cost getFHatValue() const { return g + getHHatValue(); }
    Cost getDHatValue() const { return (derr / (1.0 - curEpsilonD)); }
    Cost getHHatValue() const { return h + getDHatValue() * curEpsilonH; }

    State            getState() const { return stateRep; }
    shared_ptr<Node> getParent() const { return parent; }

    Cost         getPathBasedEpsilonH() { return curEpsilonH; }
    Cost         getPathBasedEpsilonD() { return curEpsilonD; }
    unsigned int getPathBasedExpansionCounter() { return expansionCounter; }

    void setHValue(Cost val) { h = val; }
    void setGValue(Cost val) { g = val; }
    void setDValue(Cost val) { d = val; }
    void setDErrValue(Cost val) { derr = val; }
    void setEpsilonH(Cost val) { curEpsilonH = val; }
    void setEpsilonD(Cost val) { curEpsilonD = val; }
    void setState(State s) { stateRep = s; }
    void setParent(shared_ptr<Node> p) { parent = p; }

    bool onOpen() { return open; }
    void close() { open = false; }
    void reOpen() { open = true; }

    void markStart() { stateRep.markStart(); }

    void         incDelayCntr() { ++delayCntr; }
    unsigned int getDelayCntr() { return delayCntr; }

    void pushPathBasedEpsilons(double epsH_, double epsD_)
    {
        incExpansionCounter();
        pushEpsilonHPathBased(epsH_);
        pushEpsilonHPathBased(epsD_);
    }

    void incExpansionCounter() { ++expansionCounter; }

    void pushEpsilonHPathBased(double eps)
    {
        if (expansionCounter < 5) {
            curEpsilonH = 0;
            return;
        }

        curEpsilonH -= curEpsilonH / expansionCounter;
        curEpsilonH += eps / expansionCounter;
    }

    void pushEpsilonDPathBased(double eps)
    {
        if (expansionCounter < 5) {
            curEpsilonD = 0;
            return;
        }

        curEpsilonD -= curEpsilonD / expansionCounter;
        curEpsilonD += eps / expansionCounter;
    }

    Node(Cost g_, Cost h_, Cost d_, Cost derr_, Cost epsH_, Cost epsD_,
         unsigned int expansionCounter_, State state_, shared_ptr<Node> parent_)
        : g(g_)
        , h(h_)
        , d(d_)
        , derr(derr_)
        , curEpsilonH(epsH_)
        , curEpsilonD(epsD_)
        , expansionCounter(expansionCounter_)
        , parent(parent_)
        , stateRep(state_)
    {
        open      = true;
        delayCntr = 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Node& node)
    {
        stream << node.getState() << endl;
        stream << "f: " << node.getFValue() << endl;
        stream << "g: " << node.getGValue() << endl;
        stream << "h: " << node.getHValue() << endl;
        stream << "derr: " << node.getDErrValue() << endl;
        stream << "d: " << node.getDValue() << endl;
        stream << "epsilon-h: " << node.getEpsilonH() << endl;
        stream << "epsilon-d: " << node.getEpsilonD() << endl;
        stream << "f-hat: " << node.getFHatValue() << endl;
        stream << "d-hat: " << node.getDHatValue() << endl;
        stream << "h-hat: " << node.getHHatValue() << endl;
        stream << "action generated by: " << node.getState().getLabel() << endl;
        stream << "-----------------------------------------------" << endl;
        stream << endl;
        return stream;
    }

    static bool compareNodesF(const shared_ptr<Node> n1,
                              const shared_ptr<Node> n2)
    {
        // Tie break on g-value
        if (n1->getFValue() == n2->getFValue()) {
            return n1->getGValue() > n2->getGValue();
        }
        return n1->getFValue() < n2->getFValue();
    }

    static bool compareNodesFHat(const shared_ptr<Node> n1,
                                 const shared_ptr<Node> n2)
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

    static bool compareNodesFHatFromDist(const shared_ptr<Node> n1,
                                         const shared_ptr<Node> n2)
    {
        // Tie break on f-value then g-value
        if (n1->getFHatValueFromDist() == n2->getFHatValueFromDist()) {
            if (n1->getFValue() == n2->getFValue()) {
                if (n1->getGValue() == n2->getGValue()) {
                    return n1->getState().key() > n2->getState().key();
                }
                return n1->getGValue() > n2->getGValue();
            }
            return n1->getFValue() < n2->getFValue();
        }
        return n1->getFHatValueFromDist() < n2->getFHatValueFromDist();
    }

    static bool compareNodesH(const shared_ptr<Node> n1,
                              const shared_ptr<Node> n2)
    {
        if (n1->getHValue() == n2->getHValue()) {
            return n1->getGValue() > n2->getGValue();
        }
        return n1->getHValue() < n2->getHValue();
    }

    static bool compareNodesHHat(const shared_ptr<Node> n1,
                                 const shared_ptr<Node> n2)
    {
        if (n1->getHHatValue() == n2->getHHatValue()) {
            return n1->getGValue() > n2->getGValue();
        }
        return n1->getHHatValue() < n2->getHHatValue();
    }

    static bool compareNodesBackedHHat(const shared_ptr<Node> n1,
                                       const shared_ptr<Node> n2)
    {
        /*if (n1->backupHHat == n2->backupHHat) {*/
        // return n1->getGValue() > n2->getGValue();
        /*}*/
        return n1->backupHHat < n2->backupHHat;
    }

    static double getLowerConfidence(const shared_ptr<Node> n)
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

    static bool compareNodesLC(const shared_ptr<Node> n1,
                               const shared_ptr<Node> n2)
    {
        // Lower confidence interval
        if (getLowerConfidence(n1) == getLowerConfidence(n2)) {
            return n1->getGValue() > n2->getGValue();
        }
        return getLowerConfidence(n1) < getLowerConfidence(n2);
    }
};