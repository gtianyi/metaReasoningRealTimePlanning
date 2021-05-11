#include "RealTimeSearch.h"
#include "domain/GridPathfinding.h"
#include "domain/GridPathfindingWithTarPit.h"
#include "domain/HeavyTilePuzzle.h"
#include "domain/InverseTilePuzzle.h"
#include "domain/PancakePuzzle.h"
#include "domain/RaceTrack.h"

#include <cxxopts.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

nlohmann::json parseResult(const ResultContainer&      res,
                           const cxxopts::ParseResult& args)
{

    nlohmann::json record;

    record["node expanded"]     = res.nodesExpanded;
    record["GAT node expanded"] = res.GATnodesExpanded;
    record["node generated"]    = res.nodesGenerated;
    record["solution found"]    = res.solutionFound;
    record["solution cost"]     = res.solutionCost;
    record["solution length"]   = res.solutionLength;
    record["instance"]          = args["instance"].as<std::string>();
    record["algorithm"] =
      args["alg"].as<std::string>() + "-" + args["expan"].as<std::string>();
    record["lookahead"] = args["lookahead"].as<int>();
    record["domain"]    = args["domain"].as<std::string>();
    record["subdomain"] = args["subdomain"].as<std::string>();

    return record;
}

nlohmann::json parseVisResult(const ResultContainer& res)
{

    nlohmann::json record;

    record["path"]           = res.paths;
    record["visited"]        = res.visited;
    record["isKeepThinking"] = res.isKeepThinkingFlags;
    record["committed"]      = res.committed;

    return record;
}

template<class Domain>
ResultContainer startAlg(shared_ptr<Domain> domain_ptr, string expansionModule,
                         string decisionModule, size_t lookahead)
{
    shared_ptr<RealTimeSearch<Domain>> searchAlg =
      make_shared<RealTimeSearch<Domain>>(*domain_ptr, expansionModule,
                                          decisionModule, lookahead);

    return searchAlg->search();
}

int main(int argc, char** argv)
{
    cxxopts::Options options("./realtimeSolver",
                             "This is a realtime search program");

    auto optionAdder = options.add_options();

    optionAdder(
      "d,domain", "domain type: gridPathfinding, gridPathfindingWithTarPit, tile, pancake, racetrack",
      cxxopts::value<std::string>()->default_value("gridPathfindingWithTarPit"));

    optionAdder("s,subdomain",
                "puzzle type: uniform, inverse, heavy, sqrt; "
                "pancake type: regular, heavy;"
                "racetrack map : barto-bigger, hanse-bigger-double, uniform",
                cxxopts::value<std::string>()->default_value("barto-bigger"));

    optionAdder("a,alg", "commit algorithm: one, alltheway, fhatpmr, dtrts",
                cxxopts::value<std::string>()->default_value("risk"));

    optionAdder("e,expan", "expansion algorithm: astar, fhat",
                cxxopts::value<std::string>()->default_value("risk"));

    optionAdder("l,lookahead", "expansion limit",
                cxxopts::value<int>()->default_value("100"));

    optionAdder("o,performenceOut", "performence Out file",
                cxxopts::value<std::string>());

    optionAdder("i,instance", "instance file name",
                cxxopts::value<std::string>()->default_value("2-4x4.st"));

    optionAdder("f,heuristicType",
                "gridPathfinding type : euclidean, mahattan;"
                "racetrack type : euclidean, dijkstra;"
                "pancake: gap,gapm1, gapm2",
                cxxopts::value<std::string>()->default_value("euclidean"));

    optionAdder("v,visOut", "visulization Out file",
                cxxopts::value<std::string>());

    optionAdder("h,help", "Print usage");

    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    auto domain         = args["domain"].as<std::string>();
    auto subDomain      = args["subdomain"].as<std::string>();
    auto alg            = args["alg"].as<std::string>();
    auto lookaheadDepth = static_cast<size_t>(args["lookahead"].as<int>());
    auto expan          = args["expan"].as<std::string>();

    ResultContainer res;

    if (domain == "tile") {

        /*   std::shared_ptr<SlidingTilePuzzle> world;*/

        // if (subDomain == "uniform") {
        // world = std::make_shared<SlidingTilePuzzle>(cin);
        //} else if (subDomain == "heavy") {
        // world = std::make_shared<HeavyTilePuzzle>(cin);
        //} else if (subDomain == "inverse") {
        // world = std::make_shared<InverseTilePuzzle>(cin);
        //}

        // res = startAlg<SlidingTilePuzzle>(world, alg, lookaheadDepth);

        /* } else if (domain == "pancake") {*/
        // std::shared_ptr<PancakePuzzle> world =
        // std::make_shared<PancakePuzzle>(cin);

        // if (subDomain == "heavy") {
        // world->setPuzzleVariant(1);
        //} else if (subDomain == "sumheavy") {
        // world->setPuzzleVariant(2);
        //}

        // res = startAlg<PancakePuzzle>(world, alg, lookaheadDepth);
        //} else if (domain == "racetrack") {

        // string mapFile = "/home/aifs1/gu/phd/research/workingPaper/"
        //"realtime-nancy/worlds/racetrack/map/" +
        // subDomain + ".track";

        // ifstream map(mapFile);

        // if (!map.good()) {
        // cout << "map file not exist: " << mapFile << endl;
        // exit(1);
        //}

        // std::shared_ptr<RaceTrack> world =
        // std::make_shared<RaceTrack>(map, cin);

        /*res = startAlg<RaceTrack>(world, alg, lookaheadDepth);*/
    } else if (domain == "gridPathfinding") {
        std::shared_ptr<GridPathfinding> world =
          std::make_shared<GridPathfinding>(cin);

        res = startAlg<GridPathfinding>(world, expan, alg, lookaheadDepth);
    }  else if (domain == "gridPathfindingWithTarPit") {
        std::shared_ptr<GridPathfindingWithTarPit> world =
          std::make_shared<GridPathfindingWithTarPit>(cin);

        res = startAlg<GridPathfindingWithTarPit>(world, expan, alg, lookaheadDepth);
    } else {
        cout << "Available domains are TreeWorld, slidingTile, pancake, "
                "racetrack, gridPathfinding"
             << endl;
        exit(1);
    }

    nlohmann::json record = parseResult(res, args);

    // dumpout result and observed states
    if (args.count("performenceOut")) {

        ofstream out(args["performenceOut"].as<std::string>());
        out << record;
        out.close();

    } else {
        cout << record << endl;
    }

    // dumpout solution path
    if (args.count("visOut")) {
        ofstream vout(args["visOut"].as<std::string>());
        auto     visJson = parseVisResult(res);
        vout << visJson;
        vout.close();
    }
}
