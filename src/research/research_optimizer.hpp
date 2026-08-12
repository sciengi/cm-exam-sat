#pragma once
#include <cnf/cnf.hpp>           
#include <methods/nbody.hpp>     
#include <methods/canonical_nbody.hpp>
#include <numeric/ode.hpp>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>

struct PhysicsParams {
    double c_att;      
    double c_opp;    
    double c_clause;  
    double friction;          
};

class GeneticOptimizer {
private:
    // ОПТИМИЗАЦИЯ ДАВЛЕНИЯ ГА ДЛЯ ИСКЛЮЧЕНИЯ ТАЙМАУТОВ
    static constexpr std::size_t POPULATION_SIZE = 12; 
    static constexpr std::size_t GENERATIONS = 4;

    static double evaluate_fitness(const CNF& cnf, const PhysicsParams& p, OdeMethod method, 
                                    const std::string& model_type, nbody::DecodeStrategy strategy) 
    {
        const std::size_t L = cnf.variable_count();
        const std::size_t total_clauses = cnf.clause_count();
        const double dt = 0.01;
        const std::size_t max_steps = 500; // Существенно сокращаем лимит шагов интегратора

        size_t max_satisfied_on_trajectory = 0;
        size_t step_found_sat = max_steps;
        bool sat_found = false;

        if (model_type == "2L") {
            nbody::NBodyParams params;
            params.c_att = p.c_att; params.c_opp = p.c_opp; params.c_clause = p.c_clause; params.gamma = p.friction;
            params.strategy = strategy;

            state_t state = nbody::init_state(L, params, 42);
            deriv_t deriv = nbody::build_deriv(cnf, params);
            
            some_ode_solver solver(state.size(), dt, method);
            // СГЛАЖИВАНИЕ ЖЕСТКОСТИ: Загрубляем точность, чтобы адаптивный шаг не падал в бесконечность
            solver.set_tolerances(1e-4, 1e-6); 
            double t = 0.0;

            for (size_t step = 0; step < max_steps; ++step) {
                solver.step(state, t, deriv);
                if (!state.empty() && (!std::isfinite(state[0]) || !std::isfinite(state[state.size()/2]))) break;

                if (step % 20 == 0) {
                    nbody::DecodeResult decoded = nbody::decode_best(cnf, state, params);
                    if (decoded.satisfied > max_satisfied_on_trajectory) {
                        max_satisfied_on_trajectory = decoded.satisfied;
                    }
                    if (decoded.sat) {
                        sat_found = true;
                        step_found_sat = step;
                        break;
                    }
                }
            }
        } else {
            canonical_nbody::CanonicalParams params;
            params.c_att = p.c_att; params.c_opp = p.c_opp; params.c_clause = p.c_clause; params.gamma = p.friction;

            state_t state = canonical_nbody::init_canonical_state(cnf, params, 42);
            deriv_t deriv = canonical_nbody::build_canonical_deriv(cnf, params);
            
            some_ode_solver solver(state.size(), dt, method);
            solver.set_tolerances(1e-4, 1e-6); 
            double t = 0.0;

            for (size_t step = 0; step < max_steps; ++step) {
                solver.step(state, t, deriv);
                if (!state.empty() && (!std::isfinite(state[0]) || !std::isfinite(state[state.size()/2]))) break;

                if (step % 20 == 0) {
                    CNF::model m;
                    size_t sat = canonical_nbody::decode_canonical_majority(cnf, state, params, m);
                    if (sat > max_satisfied_on_trajectory) {
                        max_satisfied_on_trajectory = sat;
                    }
                    if (sat == total_clauses) {
                        sat_found = true;
                        step_found_sat = step;
                        break;
                    }
                }
            }
        }

        double score = static_cast<double>(max_satisfied_on_trajectory) / total_clauses;
        if (sat_found) {
            double speed_bonus = static_cast<double>(max_steps - step_found_sat) / max_steps;
            score += 1.0 + speed_bonus; 
        }
        return score;
    }

public:
    static PhysicsParams optimize_parameters(const CNF& cnf, OdeMethod method, 
                                             const std::string& model_type, nbody::DecodeStrategy strategy) 
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        std::uniform_real_distribution<double> dist_att(0.3, 0.9);
        std::uniform_real_distribution<double> dist_opp(1.0, 2.2);
        std::uniform_real_distribution<double> dist_clause(1.4, 2.6);
        std::uniform_real_distribution<double> dist_friction(0.6, 1.8);

        std::vector<PhysicsParams> population(POPULATION_SIZE);
        for (auto& ind : population) {
            ind.c_att = dist_att(gen);
            ind.c_opp = dist_opp(gen);
            ind.c_clause = dist_clause(gen);
            ind.friction = dist_friction(gen);
        }

        for (size_t g = 0; g < GENERATIONS; ++g) {
            std::vector<std::pair<double, PhysicsParams>> ranked;
            ranked.reserve(POPULATION_SIZE);

            for (const auto& ind : population) {
                double fit = evaluate_fitness(cnf, ind, method, model_type, strategy);
                ranked.push_back({fit, ind});
            }

            std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
                return a.first > b.first;
            });

            std::vector<PhysicsParams> parents;
            for (size_t i = 0; i < 3; ++i) { // Берем топ-3 родителей
                parents.push_back(ranked[i].second);
            }

            population.clear();
            population.push_back(parents[0]); 

            std::uniform_int_distribution<size_t> parent_dist(0, parents.size() - 1);
            std::uniform_real_distribution<double> mutate_prob(0.0, 1.0);
            std::normal_distribution<double> mutation_delta(0.0, 0.05);

            while (population.size() < POPULATION_SIZE) {
                const auto& p1 = parents[parent_dist(gen)];
                const auto& p2 = parents[parent_dist(gen)];

                PhysicsParams child;
                child.c_att    = (p1.c_att + p2.c_att) / 2.0;
                child.c_opp    = (p1.c_opp + p2.c_opp) / 2.0;
                child.c_clause = (p1.c_clause + p2.c_clause) / 2.0;
                child.friction = (p1.friction + p2.friction) / 2.0;

                if (mutate_prob(gen) < 0.20) child.c_att    = std::clamp(child.c_att + mutation_delta(gen), 0.1, 1.5);
                if (mutate_prob(gen) < 0.20) child.c_opp    = std::clamp(child.c_opp + mutation_delta(gen), 0.4, 3.0);
                if (mutate_prob(gen) < 0.20) child.c_clause = std::clamp(child.c_clause + mutation_delta(gen), 0.8, 3.5);
                if (mutate_prob(gen) < 0.20) child.friction = std::clamp(child.friction + mutation_delta(gen), 0.1, 2.5);

                population.push_back(child);
            }
        }

        std::vector<std::pair<double, PhysicsParams>> final_ranked;
        for (const auto& ind : population) {
            final_ranked.push_back({evaluate_fitness(cnf, ind, method, model_type, strategy), ind});
        }
        std::sort(final_ranked.begin(), final_ranked.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

        return final_ranked[0].second;
    }
};