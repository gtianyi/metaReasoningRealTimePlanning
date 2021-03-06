#!/usr/bin/env python
'''
python3 script
plotting config code for generate
action commitment startegy for realtime search related plots

Author: Tianyi Gu
Date: 04/24/2021
'''

from collections import OrderedDict


class Configure:
    def __init__(self):

        self.algorithms = OrderedDict(
            {
                # "one": "ONE",
                # "alltheway": "ALL",
                # "dtrts": "Our Approach",
                "one-fhat": "ONE",
                "alltheway-fhat": "ALL",
                "dtrts-fhat": "Our Approach",
                # "one-astar": "ONE",
                # "alltheway-astar": "ALL",
                # "dtrts-astar": "Our Approach",
                # "dynamicLookahead-astar": "DynamicLookahead",
                "dynamicLookahead-fhat": "DynamicFhat",
            }
        )

        self.algorithmOrder = [self.algorithms[key] for key in self.algorithms]

        self.markers = [
        "o", "v", "s", "<", "p", "h", "^", "D", "X", ">", "o", "v", "s", "<",
        "p", "h", "^", "D", "X", ">"
        ]

        self.algorithmPalette = {
            "ONE": "royalblue",
            # "EES-slow":"orangered",
            "ALL": "orangered",
            # r"$\widehat{\mathrm{PTS}}$": "orangered",
            "Our Approach": "limegreen",
            # "WA*-slow": "orangered",
            # "BEES-LBUG": "maroon",
            # "DynamicLookahead": "deepskyblue",
            "DynamicFhat": "deepskyblue",
            # "DXES-0.8": "magenta",
            # "DXES": "maroon",
            # "DXES-NRE": "magenta",
            # "DXES-RE": "maroon",
            # "DPS": "tan",
            # "XES-LBUG": "maroon",
            # "XES-cp05": "maroon",
            # "XES-sp100": "maroon",
            # "XES-c05s100": "maroon",
            # "BEES95": "tan",
            # "RR111": "darkgreen",
            # "RR811": "yellowgreen",
            # "BEES95-cp05": "darkgreen",
            # "BEES95-sp100": "darkgreen",
            # "BEES95-c05s100": "darkgreen",
            # "XES-bf": "darkgreen",
            # "XES-OV": "maroon",
            # "PTS-OV": "deepskyblue",
            # "BEES95-OV": "gold",
            # "XES-OV-SI": "grey",
            # "PTS-OV-SI": "yellowgreen",
            # "BEES95-OV-SI": "mediumblue",
            # "BEES95-OV-SI-LBUG": "yellowgreen",
        }

        self.showname = {"nodeGen": "Total Nodes Generated",
                         "nodeExp": "Total Nodes Expanded",
                         "gatNodeGen": " GAT Nodes Expanded",
                         "nodeGenDiff": "Algorithm Node Generated /  baseline Node Generated",
                         "fixedbaseline":
                         "log10 (Algorithm Node Generated /  baseline Node Generated)",
                         "cpu": "Raw CPU Time",
                         "solved": "Number of Solved Instances (Total=totalInstance)",
                         "lookahead": "Node Expansion Limit",
                         "solutionLength": "Solution Length",
                         "gatRatio": "GAT factor of optimal",
                         }

        self.totalInstance = {"tile": "100", "pancake": "100",
                              "racetrack": "25", "vacuumworld": "60",
                              "gridPathfinding": "100",
                              "gridPathfindingWithTarPit": "100"}

        self.domainLookaheadConfig = {
                                   "avaiableLookahead": {
                                       "tile": {
                                           "uniform": [10, 30, 100, 300, 1000],
                                       },
                                       "pancake": {
                                       },
                                       "vacuumworld": {
                                       },
                                       "racetrack": {
                                       },
                                       "gridPathfinding": {
                                           "goalObstacleField": [10, 30, 100, 300, 1000],
                                           "startObstacleField": [10, 30, 100, 300, 1000],
                                           "uniformObstacleField": [10, 30, 100, 300, 1000],
                                       },
                                       "gridPathfindingWithTarPit": {
                                           "goalObstacleField": [10, 30, 100, 300, 1000],
                                           "startObstacleField": [10, 30, 100, 300, 1000],
                                           "uniformObstacleField": [10, 30, 100, 300, 1000],
                                           "goalObstacle_big_checkerboard": [4, 10, 30, 100, 300, 1000],
                                           "startObstacle_big_checkerboard": [4, 10, 30, 100, 300, 1000],
                                           "uniformObstacle_big_checkerboard": [4, 10, 30, 100, 300, 1000],
                                           "mixed_big_checkerboard": [4, 10, 30, 100, 300, 1000],
                                           "mixed_big_checkerboard_corridor": [4, 10, 30, 100, 300, 1000],
                                           "only_corridor_big_checkerboard": [4, 10, 30, 100, 300, 1000],

                                       }
                                    },
                                   }

    def getAlgorithms(self, removeAlgorithm):
        if removeAlgorithm:
            for rmAlg in removeAlgorithm:
                if rmAlg in self.algorithms:
                    self.algorithmOrder.pop(self.algorithms[rmAlg])
                    self.algorithms.pop(rmAlg)
        return self.algorithms

    def getAlgorithmsOrder(self):
        return self.algorithmOrder

    def getShowname(self):
        return self.showname

    def getTotalInstance(self):
        return self.totalInstance

    def getDomainLookaheadConfig(self):
        return self.domainLookaheadConfig

    def getAlgorithmColor(self):
        return self.algorithmPalette

    def getMarkers(self):
        return self.markers
