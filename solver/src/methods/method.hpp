#ifndef HPP_METHODS_METHOD_
#define HPP_METHODS_METHOD_

#include <vector>
#include <functional>

#include <cnf/cnf.hpp>


using state_t = std::vector<double>;
using deriv_t = std::function<void(const state_t&, state_t&)>;



struct Method {

    // WARN: fields must be filled (for example by `Cli::Parse`) before any call to struct methods

    virtual state_t InitState(size_t variable_count) = 0;

    virtual deriv_t BuildDeriv(const CNF& cnf) = 0;

    virtual bool PostProcessState(state_t& state);

    virtual void Decode(const state_t& state, CNF::model& model) = 0;

    virtual void Print(std::ostream& stream) const = 0;

    virtual ~Method() = default;
};

std::ostream& operator<<(std::ostream& stream, const Method& m); 

#endif
