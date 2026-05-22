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

# ===== Solution method module =========
set(SOURCES src/methods/solution.cpp)
add_library(method_solution STATIC ${SOURCES})
target_include_directories(method_solution PUBLIC src/methods/)

# ===== NBody method module ============
set(SOURCES src/methods/nbody.cpp)
add_library(method_nbody STATIC ${SOURCES})
target_include_directories(method_nbody PUBLIC src/methods/)

# ===== CNF test =======================
set(SOURCES src/test_cnf.cpp)
add_executable(test_cnf ${SOURCES})
target_link_libraries(test_cnf PRIVATE cnf)

# ===== Solution method deriv test =====
set(SOURCES src/test_solution_deriv.cpp)
add_executable(test_solution_deriv ${SOURCES})
target_link_libraries(test_solution_deriv PRIVATE cnf method_solution)

# ===== NBody method deriv test ========
set(SOURCES src/test_nbody_deriv.cpp)
add_executable(test_nbody_deriv ${SOURCES})
target_link_libraries(test_nbody_deriv PRIVATE cnf method_nbody)

# ===== Solver on nbody method =========
set(SOURCES src/solver_complete.cpp)
add_executable(solver_complete ${SOURCES})
target_link_libraries(solver_complete PRIVATE cnf)

# ===== Solver on solution method ======
set(SOURCES src/solver_solution.cpp)
add_executable(solver_solution ${SOURCES})
target_link_libraries(solver_solution PRIVATE cnf ode method_solution)

# ===== Solver on nbody method =========
set(SOURCES src/solver_nbody.cpp)
add_executable(solver_nbody ${SOURCES})
target_link_libraries(solver_nbody PRIVATE cnf ode method_nbody)

