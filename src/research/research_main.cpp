#include "research_optimizer.hpp"
#include <cnf/cnf.hpp>
#include <methods/nbody.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

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
    std::string method_str = get_cmd_option(args, "--method");     // RK4, RK8, DP8Adaptive

    if (filepath.empty() || model_type.empty() || method_str.empty()) {
        std::cerr << "Usage: " << argv[0] << " --file <cnf_path> --model_type <2L|3C> --strategy <HardStop|GreedySkip> --method <RK4|RK8|DP8Adaptive>\n";
        return 1;
    }

    OdeMethod method = OdeMethod::RK4;
    if (method_str == "RK8") method = OdeMethod::RK8;
    else if (method_str == "DP8Adaptive") method = OdeMethod::DP8Adaptive;

    nbody::DecodeStrategy strategy = (strategy_str == "GreedySkip") ? nbody::DecodeStrategy::GreedySkip : nbody::DecodeStrategy::HardStop;

    try {
        CNF cnf_formula(filepath);

        auto start = std::chrono::high_resolution_clock::now();
        // Запускаем новый чистый эволюционный поиск по максимуму выполненных скобок
        PhysicsParams optimized = GeneticOptimizer::optimize_parameters(cnf_formula, method, model_type, strategy);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        // Выводим CSV лог: МАРКЕР, ПУТЬ, L, МОДЕЛЬ, СТРАТЕГИЯ, ОДУ, ВРЕМЯ_ГА, C_ATT, C_OPP, C_CLAUSE, GAMMA
        std::cout << "SUCCESS_PARAM,"
                  << filepath << ","
                  << cnf_formula.variable_count() << ","
                  << model_type << ","
                  << strategy_str << ","
                  << method_str << ","
                  << duration.count() << ","
                  << optimized.c_att << ","
                  << optimized.c_opp << ","
                  << optimized.c_clause << ","
                  << optimized.friction << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "[ERR] Exception during genetic optimization: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}