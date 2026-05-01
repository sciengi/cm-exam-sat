#ifndef HPP_NUMERIC_ODE_
#define HPP_NUMERIC_ODE_

#include <methods/solution.hpp>  // DEV: move state and deriv aliases to general file

class some_ode_solver {
    public:
        some_ode_solver(size_t state_size, double initial_step);

        void operator()(state_t& state, double t, deriv_t& deriv);
};

#endif
