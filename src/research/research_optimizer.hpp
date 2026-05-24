#pragma once
#include "cnf/cnf.hpp"           
#include "methods/nbody.hpp"     
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <fstream>
#include <sstream>

struct PhysicsParams {
    double gravity_coef;      
    double repulsion_coef;    
    double clause_repulsion;  
    double friction;          
};

struct StatResult {
    double mean;
    double variance;
};

// ЭТАП 1: Мгновенное чтение Ground Truth из подготовленного датасета
class Stage1DiscreteSolver {
private:
    static inline std::unordered_map<std::string, CNF::model> cache;
    static inline bool is_loaded = false;

public:
    static void load_cache(const std::string& cache_filepath) {
        std::ifstream file(cache_filepath);
        if (!file.is_open()) {
            std::cout << "[ERR] Не удалось открыть файл с эталонами: " << cache_filepath << "\n";
            std::cout << "[I] Убедитесь, что запустили prepare_dataset.py\n";
            return;
        }

        std::string line, filename, bin_model;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            if (ss >> filename >> bin_model) {
                CNF::model m(bin_model.size());
                for (size_t i = 0; i < bin_model.size(); ++i) {
                    m[i] = (bin_model[i] == '1');
                }
                cache[filename] = m;
            }
        }
        is_loaded = true;
        std::cout << "[I] Кэш эталонов успешно загружен. Записей: " << cache.size() << "\n";
    }

    static std::optional<CNF::model> get_ground_truth(const std::string& filepath) {
        if (!is_loaded) {
            // Ожидается, что программа запускается из папки build/
            load_cache("../bench/test/ground_truth.txt"); 
        }

        size_t slash_pos = filepath.find_last_of("/\\");
        std::string filename = (slash_pos == std::string::npos) ? filepath : filepath.substr(slash_pos + 1);

        if (cache.contains(filename)) {
            return cache[filename];
        }
        
        return std::nullopt; 
    }
};

// ЭТАП 2: Генетический алгоритм
class GeneticOptimizer {
private:
    static size_t calculate_hamming_distance(const CNF::model& v1, const CNF::model& v2) {
        size_t dist = 0;
        for (size_t i = 0; i < v1.size(); ++i) {
            if (v1[i] != v2[i]) dist++;
        }
        return dist;
    }

public:
    // Обновленная функция оценки параметров с ограниченным мультизапуском
    static double evaluate_fitness(const CNF& cnf, 
                                   const CNF::model& ground_truth, 
                                   const PhysicsParams& params) 
    {
        nbody::NBodyParams nbody_p;
        nbody_p.dim = 3; 
        nbody_p.c_att = params.gravity_coef;
        nbody_p.c_opp = params.repulsion_coef;
        nbody_p.c_clause = params.clause_repulsion;
        nbody_p.gamma = params.friction;
        nbody_p.eps = 1e-2;

        // Ограничиваем мультизапуск в исследовании (например, 5 попыток),
        // чтобы алгоритм не работал слишком долго.
        const int MAX_RESEARCH_RESTARTS = 5; 
        double best_fitness_across_restarts = 0.0;

        for (int restart = 0; restart < MAX_RESEARCH_RESTARTS; ++restart) {
            // Используем переменную restart как уникальный seed для каждой попытки
            state_t state = nbody::init_state(cnf.variable_count(), nbody_p, restart);
            deriv_t deriv = nbody::build_deriv(cnf, nbody_p);

            double dt = 0.01;
            size_t max_steps = 400; 
            state_t dst(state.size(), 0.0);

            for (size_t step = 0; step < max_steps; ++step) {
                deriv(state, dst); 
                for (size_t i = 0; i < state.size(); ++i) {
                    state[i] += dst[i] * dt; 
                }
            }

            nbody::DecodeResult decode_res = nbody::decode_best(cnf, state, nbody_p);
            
            if (!decode_res.decoded) continue; 

            // Считаем качество текущей попытки
            size_t distance = calculate_hamming_distance(decode_res.model, ground_truth);
            double current_fitness = 1.0 / (static_cast<double>(distance) + 1.0);
            current_fitness += static_cast<double>(decode_res.satisfied) / cnf.clause_count();

            // Если эта попытка оказалась лучшей, запоминаем её
            if (current_fitness > best_fitness_across_restarts) {
                best_fitness_across_restarts = current_fitness;
            }

            // ЕСЛИ ФИЗИКА НАШЛА ПОЛНОЕ РЕШЕНИЕ (SAT), прерываем мультизапуск досрочно!
            // Зачем тратить время на оставшиеся попытки, если параметры уже доказали свою идеальность?
            if (decode_res.sat) {
                break; 
            }
        }

        return best_fitness_across_restarts;
    }

    static PhysicsParams optimize_parameters(const CNF& cnf, const CNF::model& ground_truth) {
        std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<double> dist_param(0.05, 3.0);

        const int POPULATION_SIZE = 15;
        const int GENERATIONS = 6;
        
        std::vector<PhysicsParams> population(POPULATION_SIZE);
        for (auto& ind : population) {
            ind = { dist_param(gen), dist_param(gen), dist_param(gen), dist_param(gen) };
        }

        for (int g = 0; g < GENERATIONS; ++g) {
            std::vector<std::pair<double, PhysicsParams>> ranked_pop;
            for (const auto& ind : population) {
                double fit = evaluate_fitness(cnf, ground_truth, ind);
                ranked_pop.push_back({fit, ind});
            }

            std::sort(ranked_pop.begin(), ranked_pop.end(), [](const auto& a, const auto& b){
                return a.first > b.first;
            });

            std::vector<PhysicsParams> next_gen;
            for (int i = 0; i < 4; ++i) next_gen.push_back(ranked_pop[i].second);

            while (next_gen.size() < POPULATION_SIZE) {
                int p1 = gen() % 4, p2 = gen() % 4;
                PhysicsParams child {
                    (ranked_pop[p1].second.gravity_coef + ranked_pop[p2].second.gravity_coef) / 2.0,
                    (ranked_pop[p1].second.repulsion_coef + ranked_pop[p2].second.repulsion_coef) / 2.0,
                    (ranked_pop[p1].second.clause_repulsion + ranked_pop[p2].second.clause_repulsion) / 2.0,
                    (ranked_pop[p1].second.friction + ranked_pop[p2].second.friction) / 2.0
                };
                if (dist_param(gen) > 2.5) child.gravity_coef += dist_param(gen) * 0.05;
                next_gen.push_back(child);
            }
            population = std::move(next_gen);
        }
        return population[0];
    }
};

class StatsAccumulator {
public:
    static StatResult calculate_stats(const std::vector<double>& values) {
        if (values.empty()) return {0.0, 0.0};
        double sum = std::accumulate(values.begin(), values.end(), 0.0);
        double mean = sum / values.size();
        double sq_sum = 0.0;
        for (double val : values) sq_sum += (val - mean) * (val - mean);
        double variance = values.size() > 1 ? sq_sum / (values.size() - 1) : 0.0;
        return {mean, variance};
    }

    static void print_report(const std::string& class_name, const std::vector<PhysicsParams>& sample) {
        std::vector<double> gravs, repuls, cls, frics;
        for (const auto& p : sample) {
            gravs.push_back(p.gravity_coef);
            repuls.push_back(p.repulsion_coef);
            cls.push_back(p.clause_repulsion);
            frics.push_back(p.friction);
        }
        auto g_stat = calculate_stats(gravs);
        auto r_stat = calculate_stats(repuls);
        auto c_stat = calculate_stats(cls);
        auto f_stat = calculate_stats(frics);

        std::cout << "=== СТАТИСТИКА КЛАССА КНФ: " << class_name << " ===\n";
        std::cout << "[STAT] Gravity (c_att): M = " << g_stat.mean << ", Var = " << g_stat.variance << "\n";
        std::cout << "[STAT] Repulsion(c_opp): M = " << r_stat.mean << ", Var = " << r_stat.variance << "\n";
        std::cout << "[STAT] Clause (c_clau): M = " << c_stat.mean << ", Var = " << c_stat.variance << "\n";
        std::cout << "[STAT] Friction (gamma): M = " << f_stat.mean << ", Var = " << f_stat.variance << "\n\n";
    }
};