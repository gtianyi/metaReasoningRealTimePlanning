add_executable(realtimeSolver main.cpp)
target_link_libraries(realtimeSolver CONAN_PKG::nlohmann_json CONAN_PKG::cxxopts)
