
#include <exception>
#include <iostream>
#include <string_view>

#include <cnf/cnf.hpp>
#include <methods/method.hpp>
#include <numeric/ode.hpp>

#include <utils/cli_parsers.hpp>


const std::string_view PREFIX_ERROR = "EE";
const std::string_view PREFIX_WARN  = "WW";
const std::string_view PREFIX_INFO  = "II";

enum RC {
    SUCCESS = 0,
    FAIL,
    NOT_IMPL,
    NO_SOLVER,
    UNKNOWN_SOLVER
};


int main(int argc, char** argv) {
  
    try {
        auto [conf, m] = Cli::Parse(argc, argv);

        std::cout 
            << conf.initial_step << '\n'
            << conf.max_step     << '\n'
            << conf.log_every    << '\n'
            << conf.target       << '\n'
            << std::endl;

    } catch(std::exception& e) {
        std::cerr << PREFIX_ERROR << ' ' << e.what() << std::endl;
    } 
  
    return 0;
    
    /*

    // TODO: 
    // - fetch args from methods special parser functions to generate help msg
    // - add flag for ode solver

    CNF cnf(conf.target);

    auto state = m->init_state (cnf.variable_count());
    auto deriv = m->build_deriv(cnf);

    CNF::model model(cnf.variable_count());
    m->decode(state, model);


    some_ode_solver nm(state.size(), conf.initial_step, OdeMethod::RK4);

    double time = 0.0;

    for (size_t step = 0; step < conf.max_step; ++step) {
        if (step % conf.log_every == 0) {  }
        // TODO: 
        // - logging routine
        // - check SAT return RC::SUCCESS
        // - np.step 
        // - state process by method (clipping state and etc) 
        //              -> add call predicate to Method to print WARN to user ???
        // - decode
    }

    // logging abot fail
    
    return RC::FAIL;
    */
}

