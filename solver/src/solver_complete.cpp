
#ifndef PROGRAM_COMPILE_NAME
#define PROGRAM_COMPILE_NAME "solver_complete"
#endif

#include <iostream>
#include <vector>
#include <cnf/cnf.hpp>


bool next(std::vector<bool>& model) {
    for (size_t i = 0; i < model.size(); ++i) {
        if (!model[i]) {
            model[i] = true;
            return true;
        }

        model[i] = false;
    }

    return false;
}


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "error: no input" << std::endl;
        std::cout << "usage: " PROGRAM_COMPILE_NAME " <input_dimacscnf>" << std::endl;
        return 1;
    }

    CNF cnf(argv[1]);
    std::vector<bool> model(cnf.variable_count(), false);

    do {
        if (cnf(model)) { 
            for (const auto& v : model) std::cout << v; 
            std::cout << std::endl; 
        }
    } while(next(model));
        
    return 0;
}
