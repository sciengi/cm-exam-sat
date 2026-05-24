cmake_minimum_required(VERSION 3.10)

project(cm-sat VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Общая папка для инклудов, чтобы не дублировать код
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ==========================================
# 1. ТЕСТОВЫЕ ПРОГРАММЫ (Компоненты и производные)
# ==========================================

# Тест парсера КНФ
add_executable(test_cnf 
    src/test_cnf.cpp 
    src/cnf/cnf.cpp
)
target_include_directories(test_cnf PRIVATE ${INCLUDE_DIR})

# Тест вычисления производных для N-Body метода
add_executable(test_nbody_deriv 
    src/test_nbody_deriv.cpp 
    src/cnf/cnf.cpp 
    src/methods/nbody.cpp
)
target_include_directories(test_nbody_deriv PRIVATE ${INCLUDE_DIR})

# Тест вычисления производных для базового метода (Solution)
add_executable(test_solution_deriv 
    src/test_solution_deriv.cpp 
    src/cnf/cnf.cpp 
    src/methods/nbody.cpp
)
target_include_directories(test_solution_deriv PRIVATE ${INCLUDE_DIR})

# ==========================================
# 2. ОСНОВНЫЕ РЕШАТЕЛИ (Solvers)
# ==========================================

# Решатель на базе метода N-Body
add_executable(solver_nbody 
    src/solver_nbody.cpp 
    src/cnf/cnf.cpp 
    src/methods/nbody.cpp 
    src/numeric/ode.cpp
)
target_include_directories(solver_nbody PRIVATE ${INCLUDE_DIR})

# ==========================================
# 3. ИССЛЕДОВАТЕЛЬСКИЙ СТЕНД (Этапы 1 и 2)
# ==========================================

# Наша программа оптимизации параметров (ГА + Статистика)
add_executable(research_bench 
    src/research/research_main.cpp 
    src/cnf/cnf.cpp 
    src/methods/nbody.cpp 
    src/methods/nbody.cpp 
    src/numeric/ode.cpp
)
target_include_directories(research_bench PRIVATE ${INCLUDE_DIR})