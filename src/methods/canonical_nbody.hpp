#pragma once
#include <cnf/cnf.hpp>
#include <numeric/ode.hpp>
#include <vector>
#include <cstddef>

namespace canonical_nbody {

struct CanonicalParams {
    std::size_t dim = 3;
    double c_att = 0.75;    // Сила притяжения одинаковых клонов-литералов
    double c_opp = 1.65;    // Сила отталкивания противоположных литералов
    double c_clause = 2.10; // Сила внутридизъюнктивного расталкивания троек
    double gamma = 1.35;    // Линейное трение среды
    double eps = 1e-2;      // Регуляризатор деления на ноль
};

struct AtomInfo {
    size_t clause_idx;
    size_t literal_pos;
    int literal;
};

// Выделение памяти и случайная инициализация 3C тел [positions(3C*dim), velocities(3C*dim)]
state_t init_canonical_state(const CNF& cnf, const CanonicalParams& params, unsigned seed);

// Генератор дифференциальных уравнений правых частей ОДУ
deriv_t build_canonical_deriv(const CNF& cnf, const CanonicalParams& params);

// Канонический декодер Матиясевича через мажоритарное голосование центров масс
size_t decode_canonical_majority(const CNF& cnf, const state_t& state, const CanonicalParams& params, CNF::model& out_model);

} // namespace canonical_nbody