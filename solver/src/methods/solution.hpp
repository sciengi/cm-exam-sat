#ifndef HPP_METHODS_SOLUTION_
#define HPP_METHODS_SOLUTION_

#include <methods/method.hpp>



struct SolutionMethod : public Method {
    
    double alpha;
    double beta;
    double lambda;
    double mu;

    state_t init_state(size_t variable_count) override;

    deriv_t build_deriv(const CNF& cnf) override;

    void decode(const state_t& state, CNF::model& model) override;
};

#endif
