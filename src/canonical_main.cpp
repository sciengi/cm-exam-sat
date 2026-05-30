#include <methods/canonical_nbody.hpp>
#include <cnf/cnf.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

std::string get_option(const std::vector<std::string>& args, const std::string& opt) {
    for (size_t i = 0; i < args.size() - 1; ++i) {
        if (args[i] == opt) return args[i + 1];
    }
    return "";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);
    std::string filepath = get_option(args, "--file");
    std::string method_str = get_option(args, "--method");

    if (filepath.empty() || method_str.empty()) {
        std::cerr << "Usage: " << argv[0] << " --file <path_to_cnf> --method <RK4|Leapfrog|DP8Adaptive>\n";
        return 1;
    }

    canonical_nbody::CanonicalParams params;
    // Используем сбалансированные канонические коэффициенты
    params.c_att = 0.75;
    params.c_opp = 1.65;
    params.c_clause = 2.10;
    params.gamma = 1.35;

    OdeMethod ode_method = OdeMethod::RK4;
    if (method_str == "Leapfrog") ode_method = OdeMethod::Leapfrog;
    else if (method_str == "DP8Adaptive") ode_method = OdeMethod::DP8Adaptive;

    try {
        CNF cnf(filepath);
        bool solved = false;
        size_t best_satisfied = 0;

        auto start = std::chrono::high_resolution_clock::now();

        // Делаем 3 перезапуска со случайных позиций (сиды 0, 1, 2)
        for (unsigned seed = 0; seed < 3; ++seed) {
            state_t state = canonical_nbody::init_canonical_state(cnf, params, seed);
            deriv_t deriv = canonical_nbody::build_canonical_deriv(cnf, params);
            
            some_ode_solver solver(state.size(), 0.01, ode_method);
            double t = 0.0;

            for (size_t step = 0; step < 1800; ++step) {
                solver.step(state, t, deriv);

                if (!state.empty() && (!std::isfinite(state[0]) || !std::isfinite(state[state.size()/2]))) {
                    break; 
                }

                if (step % 20 == 0) {
                    CNF::model m;
                    size_t sat = canonical_nbody::decode_canonical_majority(cnf, state, params, m);
                    if (sat > best_satisfied) best_satisfied = sat;
                    if (sat == cnf.clause_count()) {
                        solved = true;
                        break;
                    }
                }
            }
            if (solved) break;
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;

        // Вывод: УСПЕХ, ФАЙЛ, L, СКОБОК_РЕШЕНО, ВСЕГО_СКОБОК, ВРЕМЯ
        std::cout << (solved ? "1" : "0") << ","
                  << filepath << ","
                  << cnf.variable_count() << ","
                  << best_satisfied << ","
                  << cnf.clause_count() << ","
                  << diff.count() << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "[ERR] Exception: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}