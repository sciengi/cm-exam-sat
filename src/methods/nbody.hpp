#ifndef HPP_METHODS_NBODY_
#define HPP_METHODS_NBODY_

#include <cnf/cnf.hpp>
#include <functional>
#include <cstddef>

using state_t = std::vector<double>;
using deriv_t = std::function<void(const state_t&, state_t&)>;


namespace nbody {

// Параметры N-body модели для метода II.
struct NBodyParams {
    std::size_t dim = 3;

    // Силы:
    // c_opp    - отталкивание противоположных литералов a_i и a_{i+L}
    // c_att    - притяжение непарных атомов
    // c_clause - расталкивание троек из дизъюнктов
    // gamma    - линейное трение
    double c_opp = 1.0;
    double c_att = 0.05;
    double c_clause = 1.0;
    double gamma = 0.1;

    // eps сглаживает расстояние, чтобы не делить на 0.
    double eps = 1e-2;

    // Начальные условия.
    double initial_position_scale = 1.0;
    double initial_velocity_scale = 0.05;
};

struct DecodeResult {
    bool decoded = false;      // удалось построить M со свойством A2
    bool sat = false;          // построенная модель удовлетворяет CNF
    std::size_t satisfied = 0; // сколько дизъюнктов удовлетворено
    std::size_t start_atom = 0;
    CNF::model model;
};

std::size_t atom_count(std::size_t variable_count);

std::size_t state_size(std::size_t variable_count, std::size_t dim);

std::size_t literal_to_atom_type(int lit, std::size_t variable_count);

std::size_t opposite_type(std::size_t atom_type, std::size_t variable_count);

state_t init_state(std::size_t variable_count,
                   const NBodyParams& params,
                   unsigned seed);

deriv_t build_deriv(const CNF& cnf,
                    const NBodyParams& params);

std::size_t count_satisfied_clauses(const CNF& cnf,
                                    const CNF::model& model);

// Перебирает все стартовые атомы и возвращает лучшую модель,
// которую удалось декодировать из геометрии.
DecodeResult decode_best(const CNF& cnf,
                         const state_t& state,
                         const NBodyParams& params);

// Возвращает true, если хотя бы какое-то M построилось.
bool decode(const CNF& cnf,
            const state_t& state,
            const NBodyParams& params,
            CNF::model& model);

} // namespace nbody

#endif
