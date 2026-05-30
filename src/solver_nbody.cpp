#include <cnf/cnf.hpp>
#include <methods/nbody.hpp>
#include <numeric/ode.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <random>

std::string get_cmd_option(const std::vector<std::string>& args, const std::string& option) {
    for (size_t i = 0; i < args.size() - 1; ++i) {
        if (args[i] == option) {
            return args[i + 1];
        }
    }
    return "";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);

    // Чтение обязательных путей и режимов законов
    std::string filepath = get_cmd_option(args, "--file");
    std::string laws_str = get_cmd_option(args, "--laws");
    
    std::string method_str = get_cmd_option(args, "--method");
    std::string opt_c_att = get_cmd_option(args, "--c_att");
    std::string opt_c_opp = get_cmd_option(args, "--c_opp");
    std::string opt_c_clause = get_cmd_option(args, "--c_clause");
    std::string opt_gamma = get_cmd_option(args, "--gamma");

    if (filepath.empty() || laws_str.empty() || method_str.empty()) {
        std::cerr << "Usage: " << argv[0] << " --file <path_to_cnf> --laws <1|2|3> --method <RK4|DP8|Leapfrog|DP8Adaptive> [optional: parameters from GA]\n";
        return 1;
    }

    int laws = std::stoi(laws_str);

    OdeMethod ode_method = OdeMethod::RK4;
    if (method_str == "Euler") ode_method = OdeMethod::Euler;
    else if (method_str == "Leapfrog") ode_method = OdeMethod::Leapfrog;
    else if (method_str == "DP8") ode_method = OdeMethod::DP8;
    else if (method_str == "DP8Adaptive") ode_method = OdeMethod::DP8Adaptive;

    nbody::NBodyParams params;
    params.dim = 3;
    params.eps = 1e-2;
    
    double base_att    = opt_c_att.empty()    ? 0.62 : std::stod(opt_c_att);
    double base_opp    = opt_c_opp.empty()    ? 1.52 : std::stod(opt_c_opp);
    double base_clause = opt_c_clause.empty() ? 1.93 : std::stod(opt_c_clause);
    double base_gamma  = opt_gamma.empty()    ? 1.54 : std::stod(opt_gamma);

    if (laws == 1) {
        params.c_att = base_att;
        params.c_opp = base_opp;
        params.c_clause = 0.0; // 1 Закон: Чистая гравитация
        params.gamma = 0.0;    // Без трения
    } else if (laws == 2) {
        params.c_att = base_att;
        params.c_opp = base_opp;
        params.c_clause = 0.0; // 2 Закона: Гравитация + Диссипация (Трение)
        params.gamma = base_gamma;
    } else { // laws == 3
        params.c_att = base_att;
        params.c_opp = base_opp;
        params.c_clause = base_clause; // 3 Закона: Полный метод
        params.gamma = base_gamma;
    }


    double dt = 0.01;
    size_t max_steps = 2500;
    const int MAX_RESTARTS = 5; 

    try {
        CNF cnf(filepath);
        bool solved = false;
        int attempts_used = 0;
        
        size_t absolute_best_satisfied = 0;

        auto start_time = std::chrono::high_resolution_clock::now();

        for (int restart = 0; restart < MAX_RESTARTS; ++restart) {
            attempts_used++;
            
            state_t state = nbody::init_state(cnf.variable_count(), params, restart);
            deriv_t deriv = nbody::build_deriv(cnf, params);
            
            some_ode_solver solver(state.size(), dt, ode_method);
            if (ode_method == OdeMethod::DP8Adaptive) {
                solver.set_tolerances(1e-5, 1e-7);
            }

            double t = 0.0;
            
            std::mt19937 gen(restart + 42);
            std::uniform_real_distribution<double> kinetic_kick(-1.0, 1.0);
            size_t velocities_start = state.size() / 2;

            for (size_t step = 0; step < max_steps; ++step) {
                solver.step(state, t, deriv);

                if (!state.empty() && (!std::isfinite(state[0]) || !std::isfinite(state[state.size() / 2]))) {
                    break; 
                }

                if (step % 30 == 0) {
                    nbody::DecodeResult decoded = nbody::decode_best(cnf, state, params);
                    if (decoded.satisfied > absolute_best_satisfied) {
                        absolute_best_satisfied = decoded.satisfied;
                    }
                    if (decoded.sat) {
                        solved = true;
                        break; 
                    }
                }
            }

            nbody::DecodeResult decoded = nbody::decode_best(cnf, state, params);

            if (decoded.satisfied > absolute_best_satisfied) {
                absolute_best_satisfied = decoded.satisfied;
            }

            if (decoded.sat) {
                solved = true;
                break; 
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end_time - start_time;

        std::cout << (solved ? "1" : "0") << ","
                  << filepath << ","
                  << cnf.variable_count() << ","
                  << laws << ","
                  << attempts_used << ","
                  << diff.count() << ","
                  << absolute_best_satisfied << ","
                  << cnf.clause_count() << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "Error processing " << filepath << ": " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
