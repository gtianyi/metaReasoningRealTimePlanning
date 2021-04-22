# Metareasoing for Realtime Search

## Welcome to Metareasoing for Realtime Search project!
The purpose of the project is to implement several commitment strategies for real-time search algorithms and run benchmark with classical search domains such as Grid Pathfinding, Sliding Tiles, and Pancakes. 

## Prerequisites
We use `clang` ecosystem to compile and develop the codebase. You can install necessary components by running
```
sudo apt install clang-6.0 clang-tidy-6.0 clang-format-6.0
```

Install up-to-date CMake version. We also use `cmake-format` to keep our CMake files tidy.
```
sudo pip install cmake
sudo pip install cmake-format
```

### Conan Setup

The [Conan](https://conan.io) package manager is used to manage project's external
dependencies. This section describes the process of setting it up.  Installation is as simple as running

```
sudo pip3 install conan
```

#### Creating Profiles
We need to setup a Conan profile — a list of properties that characterize the
environment.  The following commands will create a new profile called `default` and set it up
for Ubuntu 16.04 environment.  If you are using several profiles, you may want to choose a
more descriptive name for it.
```
# create new profile called 'default'
conan profile new default --detect
# modify settings of the 'default' profile
conan profile update settings.compiler.version=5.4 default
conan profile update settings.compiler.libcxx=libstdc++11 default
```
At the moment, there exist precompiled versions of the packages needed by
the project for this particular profile:

```
os=Linux
arch=x86_64
compiler=gcc
compiler.version=5.4
compiler.libcxx=libstdc++11
build_type=Release
```

Note that you can have multiple profiles and also modify existing ones when needed.
For more details see the Conan [Getting Started](https://docs.conan.io/en/latest/getting_started.html) guide.

## Compilation
```
git clone git@github.com:gtianyi/metaReasoningRealTimePlanning.git
mkdir build_release && cd build_release
conan install ../metaReasoningRealTimePlanning --build missing
cmake -GNinja ../metaReasoningRealTimePlanning
ninja realtimeSolver 
```
For debug purpose, you can also do the following
```
cd ..
mkdir build_debug && cd build_debug
conan install ../metaReasoningRealTimePlanning --build missing
cmake -DCMAKE_BUILD_TYPE=Debug -GNinja ../metaReasoningRealTimePlanning
ninja realtimeSolver 
```

## clang-d user config
```
cd <repo dir>
ln -s ../build_release/compile_commands.json compile_commands.json
```
If you also use editor plugin such as clangd, don't forget to symlink the build flag to the root of the source tree. For more details see the clangd [prject-setup](https://clangd.llvm.org/installation.html#project-setup) guide.

## Run
```
cd <repo>
cd ../build_release
bin/realtimeSolver -h
This is a realtime search program
Usage:
  ./realtimeSolver [OPTION...]

  -d, --domain arg          domain type: gridPathfinding, tile, pancake,
                            racetrack (default: gridPathfinding)
  -s, --subdomain arg       puzzle type: uniform, inverse, heavy, sqrt;
                            pancake type: regular, heavy;racetrack map :
                            barto-bigger, hanse-bigger-double, uniform (default:
                            barto-bigger)
  -a, --alg arg             commit algorithm: one, alltheway, fhatpmr, dtrts
                            (default: alltheway)
  -l, --lookahead arg       expansion limit (default: 10)
  -o, --performenceOut arg  performence Out file
  -i, --instance arg        instance file name (default: 2-4x4.st)
  -f, --heuristicType arg   gridPathfinding type : euclidean,
                            mahattan;racetrack type : euclidean, dijkstra;pancake: gap,gapm1,
                            gapm2 (default: euclidean)
  -v, --visOut arg          visulization Out file
  -h, --help                Print usage

example command:
bin/realtimeSolver -d gridPathfinding -a one -l 10 -o outtest.json < <instance_file_dir>/gridPathfinding/goalObstacleField/10.gp
```

## Experiments Pipeline
TODO

# Problem Instances
All the problem instance file can be found [here](https://github.com/gtianyi/searchDomainInstanceFiles)

# Visualization Repo
Some visualization can be found [here](https://github.com/gtianyi/metaReasoningAnimation)

