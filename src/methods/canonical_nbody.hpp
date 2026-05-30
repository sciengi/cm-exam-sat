#pragma once
#include <cnf/cnf.hpp>
#include <numeric/ode.hpp>
#include <vector>
#include <functional>
#include <cmath>

namespace canonical_nbody {

struct CanonicalParams {
    std::size_t dim = 3;
    double c_att = 0.62;
    double c_opp = 1.52;
    double c_clause = 1.93;
    double gamma = 1.54;
    double eps = 1e-2;
};

struct CanonicalAtom {
    size_t clause_idx;
    size_t literal_pos;
    int literal; // Значение литерала (например, -1 или 3)
};

// Функция инициализации состояния из 3C тел
state_t init_canonical_state(const CNF& cnf, const CanonicalParams& params, unsigned seed);

// Расчет правых частей ОДУ (производных) по законам Матиясевича
deriv_t build_canonical_deriv(const CNF& cnf, const CanonicalParams& params);

// Мажоритарное декодирование геометрии в дискретный SAT-ответ
size_t decode_canonical_majority(const CNF& cnf, const state_t& state, const CanonicalParams& params, CNF::model& out_model);

} // namespace canonical_nbody