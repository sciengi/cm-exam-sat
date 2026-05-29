#ifndef HPP_METHODS_METHOD_
#define HPP_METHODS_METHOD_

#include <vector>
#include <functional>

#include <cnf/cnf.hpp>


using state_t = std::vector<double>;
using deriv_t = std::function<void(const state_t&, state_t&)>;



struct Method {

    // WARN: fields must be filled (for example by `Cli::Parse`) before any call to struct methods

    virtual state_t init_state(size_t variable_count) = 0;

    virtual deriv_t build_deriv(const CNF& cnf) = 0;

    virtual void decode(const state_t& state, CNF::model& model) = 0;

    virtual void Print(std::ostream& stream) const = 0;

    virtual ~Method() = default;
};

inline std::ostream& operator<<(std::ostream& stream, const Method& m) { m.Print(stream); return stream; }

#endif
