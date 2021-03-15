#include "RealTimeSearch.h"
#include "domain/GridPathfinding.h"
#include "domain/HeavyTilePuzzle.h"
#include "domain/InverseTilePuzzle.h"
#include "domain/PancakePuzzle.h"
#include "domain/RaceTrack.h"
#include "utility/cxxopts/include/cxxopts.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

void getCpuStatistic(vector<double>& lookaheadCpuTime,
                     vector<double>& percentiles, double& mean)
{
    sort(lookaheadCpuTime.begin(), lookaheadCpuTime.end());

    mean = accumulate(lookaheadCpuTime.begin(), lookaheadCpuTime.end(), 0.0) /
           static_cast<double>(lookaheadCpuTime.size());

    for (int i = 1; i <= 100; i++) {
        size_t percentID =
          static_cast<size_t>((static_cast<double>(i) / 100) *
                              static_cast<double>(lookaheadCpuTime.size() - 1));
        percentiles.push_back(lookaheadCpuTime[percentID]);
    }
}

void parseResult(ResultContainer& res, string& outString, string algName)
{
    /*if (res.solutionFound && !domain.validatePath(res.path)) {*/
    // cout << "Invalid path detected from search: " << expansionModule
    //<< endl;
    // exit(1);
    /*}*/

    outString += "\"" + algName + "\": " + to_string(res.solutionCost) +
                 ", \"epsilonHGlobal\": " + to_string(res.epsilonHGlobal) +
                 ", \"epsilonDGlobal\": " + to_string(res.epsilonDGlobal) +
                 ", \"cpu-percentiles\": [";

    vector<double> cpuPercentiles;
    double         cpuMean;

    getCpuStatistic(res.lookaheadCpuTime, cpuPercentiles, cpuMean);

    for (auto& t : cpuPercentiles) {
        outString += to_string(t) + ", ";
    }

    outString.pop_back();
    outString.pop_back();
    outString += "], \"cpu-mean\": " + to_string(cpuMean) + ", ";
}

template<class Domain>
ResultContainer startAlg(shared_ptr<Domain> domain_ptr, string decisionModule,
                         double lookahead)
{
    shared_ptr<RealTimeSearch<Domain>> searchAlg =
      make_shared<RealTimeSearch<Domain>>(*domain_ptr, decisionModule,
                                          lookahead);

    return searchAlg->search();
}

int main(int argc, char** argv)
{
    cxxopts::Options options("./realtimeSolver",
                             "This is a realtime search program");

    auto optionAdder = options.add_options();

    optionAdder("d,domain", "domain type: treeWorld, tile, pancake, racetrack",
                cxxopts::value<std::string>()->default_value("racetrack"));

    optionAdder("s,subdomain",
                "puzzle type: uniform, inverse, heavy, sqrt; "
                "pancake type: regular, heavy;"
                "racetrack map : barto-bigger, hanse-bigger-double, uniform",
                cxxopts::value<std::string>()->default_value("barto-bigger"));

    optionAdder("a,alg", "commit algorithm: one, alltheway, fhatpmr, dtrts",
                cxxopts::value<std::string>()->default_value("risk"));

    optionAdder("l,lookahead", "expansion limit",
                cxxopts::value<int>()->default_value("100"));

    optionAdder("o,performenceOut", "performence Out file",
                cxxopts::value<std::string>()->default_value("out.txt"));

    optionAdder("v,pathOut", "path Out file", cxxopts::value<std::string>());

    optionAdder("h,help", "Print usage");

    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    auto domain         = args["domain"].as<std::string>();
    auto subDomain      = args["subdomain"].as<std::string>();
    auto alg            = args["alg"].as<std::string>();
    auto lookaheadDepth = args["lookahead"].as<int>();
    auto outPerfromence = args["performenceOut"].as<string>();

    ResultContainer res;

    if (domain == "tile") {

        std::shared_ptr<SlidingTilePuzzle> world;

        if (subDomain == "uniform") {
            world = std::make_shared<SlidingTilePuzzle>(cin);
        } else if (subDomain == "heavy") {
            world = std::make_shared<HeavyTilePuzzle>(cin);
        } else if (subDomain == "inverse") {
            world = std::make_shared<InverseTilePuzzle>(cin);
        }

        res = startAlg<SlidingTilePuzzle>(world, alg, lookaheadDepth);

    } else if (domain == "pancake") {
        std::shared_ptr<PancakePuzzle> world =
          std::make_shared<PancakePuzzle>(cin);

        if (subDomain == "heavy") {
            world->setVariant(1);
        } else if (subDomain == "sumheavy") {
            world->setVariant(2);
        }

        res = startAlg<PancakePuzzle>(world, alg, lookaheadDepth);
    } else if (domain == "racetrack") {

        string mapFile = "/home/aifs1/gu/phd/research/workingPaper/"
                         "realtime-nancy/worlds/racetrack/map/" +
                         subDomain + ".track";

        ifstream map(mapFile);

        if (!map.good()) {
            cout << "map file not exist: " << mapFile << endl;
            exit(1);
        }

        std::shared_ptr<RaceTrack> world =
          std::make_shared<RaceTrack>(map, cin);

        res = startAlg<RaceTrack>(world, alg, lookaheadDepth);
    } else if (domain == "gridPathfinding") {

        /*string mapFile =*/
        //"/home/aifs1/gu/phd/research/workingPaper/"
        //"realtime-nancy/worlds/gridPathfinding/exampleworlds/small-1.gp";
        ////+ subDomain + ".gp";

        // ifstream map(mapFile);

        // if (!map.good()) {
        // cout << "map file not exist: " << mapFile << endl;
        // exit(1);
        /*}*/

        std::shared_ptr<GridPathfinding> world =
          std::make_shared<GridPathfinding>(cin);

        res = startAlg<GridPathfinding>(world, alg, lookaheadDepth);
    } else {
        cout << "Available domains are TreeWorld, slidingTile, pancake, "
                "racetrack, gridPathfinding"
             << endl;
        exit(1);
    }

    string outString = "{ ";

    parseResult(res, outString, alg);

    outString += "\"Lookahead\": " + to_string(lookaheadDepth) + " }";

    ofstream out(outPerfromence);

    out << outString;
    out.close();

    // dumpout solution path
    if (args.count("pathOut")) {
        ofstream pout(args["pathOut"].as<std::string>());
        while (!res.path.empty()) {
            pout << res.path.front();
            res.path.pop();
        }
        pout.close();
    }
}
