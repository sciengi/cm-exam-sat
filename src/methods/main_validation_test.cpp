#include <cnf/cnf.hpp>
#include <methods/nbody.hpp>
#include <methods/canonical_nbody.hpp>
#include <numeric/ode.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <chrono>

void execute_test_for_file(const std::string& filepath) {
    std::cout << "[FILE] Тестирование файла КНФ: " << filepath << "\n";

    try {
        CNF cnf(filepath);
        size_t total_clauses = cnf.clause_count();
        size_t L = cnf.variable_count();
        double dt = 0.01;
        size_t steps_limit = 2000;

        // ================= ТЕСТ 1. УПРОЩЕННЫЙ (2L тел) + СТРАТЕГИЯ GREEDY SKIP =================
        {
            nbody::NBodyParams p_skip;
            p_skip.strategy = nbody::DecodeStrategy::GreedySkip; // Старый метод (Continue)
            
            state_t state = nbody::init_state(L, p_skip, 42);
            deriv_t deriv = nbody::build_deriv(cnf, p_skip);
            some_ode_solver solver(state.size(), dt, OdeMethod::RK8);
            double t = 0.0;
            size_t best_sat = 0;
            bool success = false;

            for (size_t step = 0; step < steps_limit; ++step) {
                solver.step(state, t, deriv);
                if (step % 20 == 0) {
                    nbody::DecodeResult res = nbody::decode_best(cnf, state, p_skip);
                    if (res.satisfied > best_sat) best_sat = res.satisfied;
                    if (res.sat) { success = true; break; }
                }
            }
            std::cout << " [2L GreedySkip]  Успех: " << (success ? "ДА" : "НЕТ")
                      << " | Лучший SAT: " << best_sat << "/" << total_clauses << "\n";
        }

        // ================= ТЕСТ 2. УПРОЩЕННЫЙ (2L тел) + СТРАТЕГИЯ HARD СТOП =================
        {
            nbody::NBodyParams p_stop;
            p_stop.strategy = nbody::DecodeStrategy::HardStop; // Новый метод (Break)
            
            state_t state = nbody::init_state(L, p_stop, 42);
            deriv_t deriv = nbody::build_deriv(cnf, p_stop);
            some_ode_solver solver(state.size(), dt, OdeMethod::RK8);
            double t = 0.0;
            size_t best_sat = 0;
            bool success = false;

            for (size_t step = 0; step < steps_limit; ++step) {
                solver.step(state, t, deriv);
                if (step % 20 == 0) {
                    nbody::DecodeResult res = nbody::decode_best(cnf, state, p_stop);
                    if (res.satisfied > best_sat) best_sat = res.satisfied;
                    if (res.sat) { success = true; break; }
                }
            }
            std::cout << " [2L HardStop]    Успех: " << (success ? "ДА" : "НЕТ")
                      << " | Лучший SAT: " << best_sat << "/" << total_clauses << "\n";
        }

        // ================= ТЕСТ 3. КАНОНИЧЕСКИЙ МЕТОД (3C тел) =================
        {
            canonical_nbody::CanonicalParams p_canonical;
            state_t state = canonical_nbody::init_canonical_state(cnf, p_canonical, 42);
            deriv_t deriv = canonical_nbody::build_canonical_deriv(cnf, p_canonical);
            some_ode_solver solver(state.size(), dt, OdeMethod::RK8);
            double t = 0.0;
            size_t best_sat = 0;
            bool success = false;

            for (size_t step = 0; step < steps_limit; ++step) {
                solver.step(state, t, deriv);
                if (step % 20 == 0) {
                    CNF::model m;
                    size_t sat = canonical_nbody::decode_canonical_majority(cnf, state, p_canonical, m);
                    if (sat > best_sat) best_sat = sat;
                    if (sat == total_clauses) { success = true; break; }
                }
            }
            std::cout << " [3C Каноника]    Успех: " << (success ? "ДА" : "НЕТ")
                      << " | Лучший SAT: " << best_sat << "/" << total_clauses << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << " [ERR] Ошибка: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "СРАВНИТЕЛЬНЫЙ ВЫЧМАТ-АНАЛИЗ ТРЕХ МЕТОДОВ ДЕКОДИРОВАНИЯ СИСТЕМ      \n";

    execute_test_for_file("bench/test/l3/uf3-1.cnf");
    execute_test_for_file("bench/test/l6/uf6-1.cnf");
    execute_test_for_file("bench/test/l9/uf9-1.cnf");

    return 0;
}