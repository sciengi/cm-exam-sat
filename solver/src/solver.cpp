
#include <exception>
#include <iostream>
#include <string_view>

#include <cnf/cnf.hpp>
#include <methods/method.hpp>
#include <numeric/ode.hpp>

#include <utils/cli_parsers.hpp>


const std::string_view PREFIX_RESULT = "RR";
const std::string_view PREFIX_ERROR  = "EE";
const std::string_view PREFIX_INFO   = "II";


std::ostream& operator<<(std::ostream& stream, const CNF::model& model) {
    for (const auto& v : model) stream << v;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const state_t& state) {
    for (size_t i = 0; i < state.size() - 1; i++) stream << state[i] << ' ';
    return stream << state.back();
}



int main(int argc, char** argv) {
  
    try {
        auto [conf, m] = Cli::Parse(argc, argv);

        std::cerr << PREFIX_INFO << " Solver config: " << conf << std::endl;
        std::cerr << PREFIX_INFO << " Method config: " << *m   << std::endl;


        CNF cnf(conf.target);

        auto state = m->InitState (cnf.variable_count());
        auto deriv = m->BuildDeriv(cnf);

        CNF::model model(cnf.variable_count());
        m->Decode(state, model);

        some_ode_solver nm(state.size(), conf.initial_step, OdeMethod::RK4); // NOTICE: ODE solver fixed yet

        double time = 0.0;

        std::cerr << "===== PROTOCOL ====="      << std::endl;
        std::cerr << "tag step time model state" << std::endl;
        for (size_t step = 0; step < conf.max_step; ++step) {
            if (step % conf.log_every == 0)
                std::cerr << PREFIX_INFO << ' ' << step << ' ' << time << " '" << model << "' " << state << std::endl; 

            if (cnf(model)) {
                std::cerr << PREFIX_RESULT << " SAT " << model << std::endl;
                return 0;
            }

            nm.step(state, time, deriv);

            if(m->PostProcessState(state)) {
                std::cerr << PREFIX_ERROR << " method signaled that state broken" << std::endl;
                return 3;
            }
            
            if (step % conf.decode_every == 0)
                m->Decode(state, model);
        }

        std::cerr << PREFIX_RESULT << " FAIL max step reached" << std::endl;    
        return 1;

    } catch(std::exception& e) {
        std::cerr << PREFIX_ERROR << ' ' << e.what() << std::endl;
        return 2;
    } 
}

