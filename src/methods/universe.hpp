#ifndef HPP_METHODS_UNIVERSE_
#define HPP_METHODS_UNIVERSE_

#include <cnf/cnf.hpp>

#include <vector>
#include <functional>


using state_t = std::vector<double>;
using deriv_t = std::function<void(const state_t&, state_t&)>;



state_t init_state(size_t variable_count);


deriv_t build_deriv(const CNF& cnf, double c1, double c2, double c3, double gamma);


void decode(const state_t& state, CNF::model& model);

#endif
