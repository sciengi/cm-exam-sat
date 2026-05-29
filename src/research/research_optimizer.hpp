#pragma once
#include <cnf/cnf.hpp>           
#include <methods/nbody.hpp>     
#include <numeric/ode.hpp>
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
#include <filesystem>

struct PhysicsParams {
    double gravity_coef;      
    double repulsion_coef;    
    double clause_repulsion;  
    double friction;          
};

class Stage1DiscreteSolver {
private:
    static inline std::unordered_map<std::string, CNF::model> cache;
    static inline bool is_loaded = false;

public:
    static void load_cache(const std::string& cache_filepath) {
        std::ifstream file(cache_filepath);
        if (!file.is_open()) {
            std::cerr << "[ERR] Не удалось открыть файл с эталонами: " << cache_filepath << "\n";
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
    }

    static std::optional<CNF::model> get_ground_truth(const std::string& filepath) {
        if (!is_loaded) return std::nullopt;
        
        std::filesystem::path p(filepath);
        std::string filename = p.filename().string();
        
        auto it = cache.find(filename);
        if (it != cache.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};

class GeneticOptimizer {
private:
    static inline double calculate_hamming_distance(const CNF::model& m1, const CNF::model& m2) {
        if (m1.size() != m2.size()) return static_cast<double>(std::max(m1.size(), m2.size()));
        double dist = 0.0;
        for (size_t i = 0; i < m1.size(); ++i) {
            if (m1[i] != m2[i]) dist += 1.0;
        }
        return dist;
    }

    static double evaluate_fitness(const CNF& cnf, const CNF::model& ground_truth, const PhysicsParams& p, OdeMethod method) {
        nbody::NBodyParams params;
        params.dim = 3;
        params.c_att = p.gravity_coef;
        params.c_opp = p.repulsion_coef;
        params.c_clause = p.clause_repulsion;
        params.gamma = p.friction;
        params.eps = 1e-2;

        double dt = 0.01;
        size_t max_steps = 600;

        state_t state = nbody::init_state(cnf.variable_count(), params, 42);
        deriv_t deriv = nbody::build_deriv(cnf, params);
        
        some_ode_solver solver(state.size(), dt, method);
        if (method == OdeMethod::DP8Adaptive) {
            solver.set_tolerances(1e-4, 1e-6);
        }
        double t = 0.0;

        size_t best_satisfied = 0;
        double min_hamming = static_cast<double>(cnf.variable_count());

        for (size_t step = 0; step < max_steps; ++step) {
            solver.step(state, t, deriv);

            // Предохранитель от взрыва ОДУ (NaN / Inf)
            if (!state.empty() && (!std::isfinite(state[0]) || !std::isfinite(state[state.size() / 2]))) {
                break; 
            }

            if (step % 20 == 0) {
                nbody::DecodeResult decoded = nbody::decode_best(cnf, state, params);
                
                if (decoded.sat) {
                    return 15.0 + (max_steps - step); // Весомый бонус за полный SAT
                }

                if (decoded.satisfied > best_satisfied) {
                    best_satisfied = decoded.satisfied;
                    min_hamming = calculate_hamming_distance(decoded.model, ground_truth);
                }
            }
        }

        double part_sat = static_cast<double>(best_satisfied) / cnf.clause_count();
        double part_hamming = 1.0 / (min_hamming + 1.0);
        return part_sat + part_hamming;
    }

public:
    static PhysicsParams optimize_parameters(const CNF& cnf, const CNF::model& ground_truth, OdeMethod method) {
        const int POPULATION_SIZE = 20;
        const int GENERATIONS = 6;
        
        std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
        
        // СУЖЕННЫЕ КОМПАКТНЫЕ ДИАПАЗОНЫ ДЛЯ СВЕРХМАЛЫХ КНФ (L <= 20)
        std::uniform_real_distribution<double> dist_gravity(0.01, 1.2);
        std::uniform_real_distribution<double> dist_repulsion(0.1, 4.0);
        std::uniform_real_distribution<double> dist_clause(0.1, 4.0);
        std::uniform_real_distribution<double> dist_friction(0.1, 3.0);

        std::vector<PhysicsParams> population(POPULATION_SIZE);
        for (auto& ind : population) {
            ind.gravity_coef = dist_gravity(gen);
            ind.repulsion_coef = dist_repulsion(gen);
            ind.clause_repulsion = dist_clause(gen);
            ind.friction = dist_friction(gen);
        }

        for (int g = 0; g < GENERATIONS; ++g) {
            std::vector<std::pair<double, PhysicsParams>> scored_pop;
            for (const auto& ind : population) {
                double score = evaluate_fitness(cnf, ground_truth, ind, method);
                scored_pop.push_back({score, ind});
            }

            std::sort(scored_pop.begin(), scored_pop.end(), [](const auto& a, const auto& b) {
                return a.first > b.first;
            });

            std::vector<PhysicsParams> parents;
            for (int i = 0; i < 4; ++i) {
                parents.push_back(scored_pop[i].second);
            }

            population.clear();
            population.push_back(parents[0]); // Сохраняем элиту

            std::uniform_int_distribution<int> parent_dist(0, parents.size() - 1);
            std::uniform_real_distribution<double> mutate_prob(0.0, 1.0);
            std::normal_distribution<double> mutation_delta(0.0, 0.1);

            while (population.size() < POPULATION_SIZE) {
                const auto& p1 = parents[parent_dist(gen)];
                const auto& p2 = parents[parent_dist(gen)];

                PhysicsParams child;
                child.gravity_coef = (p1.gravity_coef + p2.gravity_coef) / 2.0;
                child.repulsion_coef = (p1.repulsion_coef + p2.repulsion_coef) / 2.0;
                child.clause_repulsion = (p1.clause_repulsion + p2.clause_repulsion) / 2.0;
                child.friction = (p1.friction + p2.friction) / 2.0;

                if (mutate_prob(gen) < 0.2) child.gravity_coef = std::clamp(child.gravity_coef + mutation_delta(gen), 0.01, 1.2);
                if (mutate_prob(gen) < 0.2) child.repulsion_coef = std::clamp(child.repulsion_coef + mutation_delta(gen), 0.1, 4.0);
                if (mutate_prob(gen) < 0.2) child.clause_repulsion = std::clamp(child.clause_repulsion + mutation_delta(gen), 0.1, 4.0);
                if (mutate_prob(gen) < 0.2) child.friction = std::clamp(child.friction + mutation_delta(gen), 0.1, 3.0);

                population.push_back(child);
            }
        }
        return population[0];
    }
};