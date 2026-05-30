
#include <cnf/cnf.hpp>
#include <methods/nbody.hpp>
#include <numeric/ode.hpp>

#include <string>
#include <iostream>
#include <vector>

#include <iomanip>


int main() {

    double alpha, beta, lambda, mu;
    std::string dimacs_path;

    std::cin 
        >> dimacs_path 
        >> alpha  >> beta
        >> lambda >> mu;
    

    CNF cnf(dimacs_path);

    auto state = init_state(cnf.variable_count());
    auto deriv = build_deriv(cnf, alpha, beta, lambda, mu);
   
    CNF::model model(cnf.variable_count());
    decode(state, model);

    std::cout << "Initial state: " << std::showpos << std::fixed << std::setprecision(2);
    for (size_t i = 0; i < state.size() - 1; i++) std::cout << state[i] << ' ';
    std::cout << state.back() << std::endl;

    state_t state_deriv(state.size());
    deriv(state, state_deriv);

    std::cout << "Derivative:    ";
    for (size_t i = 0; i < state_deriv.size() - 1; i++) std::cout << state_deriv[i] << ' ';
    std::cout << state_deriv.back() << std::endl;
}

