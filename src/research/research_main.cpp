#include "research_optimizer.hpp"
#include "cnf/cnf.hpp"
#include <iostream>
#include <vector>
#include <string>

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

    std::string filepath = get_cmd_option(args, "--file");
    std::string cache_path = get_cmd_option(args, "--cache");
    std::string method_str = get_cmd_option(args, "--method");

    if (filepath.empty() || cache_path.empty() || method_str.empty()) {
        std::cerr << "Usage: " << argv[0] << " --file <cnf_path> --cache <ground_truth_path> --method <RK4|DP8|Leapfrog|DP8Adaptive>\n";
        return 1;
    }

    OdeMethod method = OdeMethod::RK4;
    if (method_str == "Euler") method = OdeMethod::Euler;
    else if (method_str == "Leapfrog") method = OdeMethod::Leapfrog;
    else if (method_str == "DP8") method = OdeMethod::DP8;
    else if (method_str == "DP8Adaptive") method = OdeMethod::DP8Adaptive;

    try {
        Stage1DiscreteSolver::load_cache(cache_path);

        CNF cnf_formula(filepath);
        auto ground_truth = Stage1DiscreteSolver::get_ground_truth(filepath);

        if (!ground_truth.has_value()) {
            std::cerr << "[ERR] Эталон решения для " << filepath << " не найден в кэше!\n";
            return 1;
        }

        // Замеряем время работы ГА
        auto start = std::chrono::high_resolution_clock::now();
        PhysicsParams optimized = GeneticOptimizer::optimize_parameters(cnf_formula, ground_truth.value(), method);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        // Выводим структурированный CSV лог: 
        // МАРКЕР, ПУТЬ, L, МЕТОД, ВРЕМЯ_ГА, C_ATT, C_OPP, C_CLAUSE, GAMMA
        std::cout << "SUCCESS_PARAM,"
                  << filepath << ","
                  << cnf_formula.variable_count() << ","
                  << method_str << ","
                  << duration.count() << ","
                  << optimized.gravity_coef << ","
                  << optimized.repulsion_coef << ","
                  << optimized.clause_repulsion << ","
                  << optimized.friction << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "[ERR] Исключение: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}