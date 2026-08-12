#include <cnf/cnf.hpp>
#include <methods/nbody.hpp>
#include <methods/canonical_nbody.hpp>
#include <numeric/ode.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

std::string get_cmd_option(const std::vector<std::string>& args, const std::string& option) {
    for (size_t i = 0; i < args.size() - 1; ++i) {
        if (args[i] == option) return args[i + 1];
    }
    return "";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);

    std::string filepath = get_cmd_option(args, "--file");
    std::string model_type = get_cmd_option(args, "--model_type"); // 2L или 3C
    std::string strategy_str = get_cmd_option(args, "--strategy"); // GreedySkip или HardStop
    std::string method_str = get_cmd_option(args, "--method");     // RK4, RK8, DP8, DP8Adaptive

    // Считывание физических коэффициентов сил
    double c_att = std::stod(get_cmd_option(args, "--c_att"));
    double c_opp = std::stod(get_cmd_option(args, "--c_opp"));
    double c_clause = std::stod(get_cmd_option(args, "--c_clause"));
    double gamma = std::stod(get_cmd_option(args, "--gamma"));

    if (filepath.empty() || model_type.empty() || method_str.empty()) {
        std::cerr << "ERR: Missing required arguments.\n";
        return 1;
    }

    // Парсинг вычмат-метода
    OdeMethod ode_method = OdeMethod::RK4;
    if (method_str == "RK8") ode_method = OdeMethod::RK8;
    else if (method_str == "DP8Adaptive") ode_method = OdeMethod::DP8Adaptive;

    try {
        CNF cnf(filepath);
        size_t total_clauses = cnf.clause_count();
        size_t L = cnf.variable_count();
        double dt = 0.01;
        size_t max_steps = 1500;
        
        bool solved = false;
        size_t absolute_best_satisfied = 0;

        auto start_time = std::chrono::high_resolution_clock::now();

        if (model_type == "2L") {
            // ================= РЕЖИМ 2L УПРОЩЕННЫЙ =================
            nbody::NBodyParams params;
            params.c_att = c_att; params.c_opp = c_opp; params.c_clause = c_clause; params.gamma = gamma;
            params.strategy = (strategy_str == "GreedySkip") ? nbody::DecodeStrategy::GreedySkip : nbody::DecodeStrategy::HardStop;

            state_t state = nbody::init_state(L, params, 42);
            deriv_t deriv = nbody::build_deriv(cnf, params);
            some_ode_solver solver(state.size(), dt, ode_method);
            double t = 0.0;

            for (size_t step = 0; step < max_steps; ++step) {
                solver.step(state, t, deriv);
                if (!state.empty() && (!std::isfinite(state[0]) || !std::isfinite(state[state.size()/2]))) break;

                if (step % 25 == 0) {
                    nbody::DecodeResult decoded = nbody::decode_best(cnf, state, params);
                    if (decoded.satisfied > absolute_best_satisfied) absolute_best_satisfied = decoded.satisfied;
                    if (decoded.sat) { solved = true; break; }
                }
            }
        } else if (model_type == "3C") {
            // ================= РЕЖИМ 3С КАНОНИЧЕСКИЙ =================
            canonical_nbody::CanonicalParams params;
            params.c_att = c_att; params.c_opp = c_opp; params.c_clause = c_clause; params.gamma = gamma;

            state_t state = canonical_nbody::init_canonical_state(cnf, params, 42);
            deriv_t deriv = canonical_nbody::build_canonical_deriv(cnf, params);
            some_ode_solver solver(state.size(), dt, ode_method);
            double t = 0.0;

            for (size_t step = 0; step < max_steps; ++step) {
                solver.step(state, t, deriv);
                if (!state.empty() && (!std::isfinite(state[0]) || !std::isfinite(state[state.size()/2]))) break;

                if (step % 25 == 0) {
                    CNF::model m;
                    size_t sat = canonical_nbody::decode_canonical_majority(cnf, state, params, m);
                    if (sat > absolute_best_satisfied) absolute_best_satisfied = sat;
                    if (sat == total_clauses) { solved = true; break; }
                }
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end_time - start_time;

        // Вывод строго одной плоской строки CSV: Is_Solved,Sat_Clauses,Total_Clauses,Execution_Time
        std::cout << (solved ? "1" : "0") << ","
                  << absolute_best_satisfied << ","
                  << total_clauses << ","
                  << diff.count() << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "Execution error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}