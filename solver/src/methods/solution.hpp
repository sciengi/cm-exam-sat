#ifndef HPP_METHODS_SOLUTION_
#define HPP_METHODS_SOLUTION_

#include <methods/method.hpp>



struct SolutionMethod : public Method {
    
    double alpha;
    double beta;
    double lambda;
    double mu;

    state_t InitState(size_t variable_count) override;

    deriv_t BuildDeriv(const CNF& cnf) override;

    bool PostProcessState(state_t& state) override;

    void Decode(const state_t& state, CNF::model& model) override;

    void Print(std::ostream& stream) const override;
};

#endif
