#include "research_optimizer.hpp"
#include "cnf/cnf.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::cout << "[I] Запуск автоматизированного исследовательского стенда...\n";

    std::string target_dir = "../bench/test/";
    
    // Список размерностей, которые мы хотим исследовать за один запуск
    std::vector<std::string> prefixes_to_test = {"uf50", "uf75", "uf100"};
    
    // Ограничиваем выборку 10 файлами на каждый размер, чтобы не ждать вечность
    const int max_files_to_test = 10; 

    // Проверка существования папки
    if (!fs::exists(target_dir)) {
        std::cout << "[ERR] Папка " << target_dir << " не найдена!\n";
        return 1;
    }

    // Главный цикл по разным классам сложности
    for (const auto& target_prefix : prefixes_to_test) {
        std::cout << "\n======================================================\n";
        std::cout << "[I] НАЧАЛО АНАЛИЗА КЛАССА: " << target_prefix << "\n";
        std::cout << "======================================================\n";

        std::vector<std::string> test_files;
        int count = 0;

        // Ищем нужные файлы для текущего префикса
        for (const auto& entry : fs::directory_iterator(target_dir)) {
            std::string filename = entry.path().filename().string();
            // Ищем точное совпадение начала имени (например, uf75)
            if (entry.path().extension() == ".cnf" && filename.find(target_prefix) == 0) {
                test_files.push_back(entry.path().string());
                if (++count >= max_files_to_test) break;
            }
        }

        if (test_files.empty()) {
            std::cout << "[ERR] Файлы для " << target_prefix << " не найдены. Пропускаем.\n";
            continue;
        }

        std::cout << "[I] Найдено файлов: " << test_files.size() << "\n";
        std::vector<PhysicsParams> successful_params_sample;

        for (const auto& filepath : test_files) {
            std::cout << "[I]--------------------------------------------------\n";
            std::cout << "[I] Обработка: " << filepath << "\n";
            
            try {
                CNF cnf_formula(filepath);
                
                // Берем ответ из кэша, созданного Python
                auto ground_truth = Stage1DiscreteSolver::get_ground_truth(filepath);

                if (!ground_truth.has_value()) {
                    std::cout << "[ERR] Пропуск: Эталон не найден в кэше.\n";
                    continue;
                }
                
                // 3 прогона на один файл для точной статистики
                const int RUNS_PER_CNF = 3; 
                for (int r = 0; r < RUNS_PER_CNF; ++r) {
                    PhysicsParams optimized = GeneticOptimizer::optimize_parameters(cnf_formula, ground_truth.value());
                    successful_params_sample.push_back(optimized);
                }

            } catch (const std::exception& ex) {
                std::cout << "[ERR] Ошибка при обработке: " << ex.what() << "\n";
            }
        }

        // Вывод итоговой статистики для текущего класса
        std::cout << "[I]--------------------------------------------------\n";
        if (!successful_params_sample.empty()) {
            StatsAccumulator::print_report("Benchmark-Class-" + target_prefix, successful_params_sample);
        } else {
            std::cout << "[ERR] Нет данных для отчета по " << target_prefix << "\n";
        }
    }

    std::cout << "\n[I] Все классы успешно исследованы!\n";
    return 0;
}