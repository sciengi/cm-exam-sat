cmake_minimum_required(VERSION 3.10)

project(cm-sat VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(src/)



# ===== CNF module =====================
set(SOURCES src/cnf/cnf.cpp )
add_library(cnf STATIC ${SOURCES})
target_include_directories(cnf PUBLIC src/cnf/)

# ===== Ode solvers module =============
set(SOURCES src/numeric/ode.cpp)
add_library(ode STATIC ${SOURCES})
target_include_directories(ode PUBLIC src/ode/)

# ===== Method module =========
set(SOURCES src/methods/method.cpp)
add_library(method STATIC ${SOURCES})
target_include_directories(method PUBLIC src/methods/)

# ===== Solution method module =========
set(SOURCES src/methods/solution.cpp)
add_library(method_solution STATIC ${SOURCES})
target_include_directories(method_solution PUBLIC src/methods/)
target_link_libraries(method_solution PRIVATE method)

# ===== NBody method module ============
set(SOURCES src/methods/nbody.cpp)
add_library(method_nbody STATIC ${SOURCES})
target_include_directories(method_nbody PUBLIC src/methods/)
target_link_libraries(method_nbody PRIVATE method)

# ===== Solver random ==================
set(SOURCES src/solver_random.cpp)
add_executable(solver_random ${SOURCES})
target_link_libraries(solver_random PRIVATE cnf)

# ===== Solver complete ================
set(SOURCES src/solver_complete.cpp)
add_executable(solver_complete ${SOURCES})
target_link_libraries(solver_complete PRIVATE cnf)

# ===== Solver =========================
set(SOURCES src/solver.cpp src/utils/cli_parsers.cpp)
add_executable(solver ${SOURCES})
target_link_libraries(solver PRIVATE method ode method_solution method_nbody cnf)  # BUG: ld cant find cnf lib if it first
