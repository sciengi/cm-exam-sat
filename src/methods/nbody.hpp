#ifndef HPP_METHODS_NBODY_
#define HPP_METHODS_NBODY_

#include <cnf/cnf.hpp>
#include <functional>
#include <cstddef>
#include <vector>

using state_t = std::vector<double>;
using deriv_t = std::function<void(const state_t&, state_t&)>;

namespace nbody {

// Перечисление для выбора стратегии жадного декодера (2L тел)
enum class DecodeStrategy {
    GreedySkip,  // Старый метод: просто пропускает конфликтный атом (Continue)
    HardStop     // Ваша новая идея: останавливает сборку при первом конфликте (Break)
};

struct NBodyParams {
    std::size_t dim = 3;
    double c_opp = 1.52;    
    double c_att = 0.62;
    double c_clause = 1.93;
    double gamma = 1.54;
    double eps = 1e-2;

    double initial_position_scale = 1.0;
    double initial_velocity_scale = 0.05;

    // Стратегия декодирования по умолчанию
    DecodeStrategy strategy = DecodeStrategy::HardStop; 
};

struct DecodeResult {
    bool decoded = false;      
    bool sat = false;          
    std::size_t satisfied = 0; 
    std::size_t start_atom = 0;
    CNF::model model;
};

std::size_t state_size(std::size_t variable_count, std::size_t dim);
std::size_t literal_to_atom_type(int lit, std::size_t variable_count);
std::size_t opposite_type(std::size_t atom_type, std::size_t variable_count);

state_t init_state(std::size_t variable_count, const NBodyParams& params, unsigned seed);
deriv_t build_deriv(const CNF& cnf, const NBodyParams& params);
std::size_t count_satisfied_clauses(const CNF& cnf, const CNF::model& model);

DecodeResult decode_best(const CNF& cnf, const state_t& state, const NBodyParams& params);

} // namespace nbody

#endif // HPP_METHODS_NBODY_