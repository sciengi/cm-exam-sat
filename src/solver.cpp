
#include <cnf/cnf.hpp>
#include <methods/solution.hpp>
#include <numeric/some_ode_solver.hpp>

#include <string>
#include <iostream>
#include <vector>



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
    
    const double initial_step = 0.001;
    some_ode_solver nm(state.size(), initial_step);

    CNF::model model(cnf.variable_count());
    decode(state, model);

    double t = 0.0, time_step = 0.1;
    while (cnf(model) != true) {

        nm(state, t, deriv);

        decode(state, model);
        
        t += time_step;
    }

    // DEV: add some callbacks, limit for time, etc 

    std::cout << "SAT ";
    for (size_t i = 0; i < model.size() - 1; i++) 
        std::cout << model[i] << ' ';
    std::cout << model.back() << std::endl;
}






