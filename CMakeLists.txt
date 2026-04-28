cmake_minimum_required(VERSION 3.10)

project(cm-sat VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# === Temporary CNF test program ===

set(TEST_CNF test_cnf)

add_executable(${TEST_CNF} src/test_cnf.cpp src/cnf/cnf.cpp)

target_include_directories(${TEST_CNF} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


# === Solver program ===

set(SOLVER solver)

set(SOURCES
    src/solver.cpp
)

add_executable(${SOLVER} ${SOURCES})

target_include_directories(${SOLVER} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

