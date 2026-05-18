#include <cnf/cnf.hpp>
#include <methods/nbody.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: test_nbody_deriv <dimacs_path>\n";
        return 1;
    }

    CNF cnf(argv[1]);

    nbody::NBodyParams params;
    params.dim = 3;
    params.c_opp = 1.0;
    params.c_att = 0.05;
    params.c_clause = 1.0;
    params.gamma = 0.1;

    auto state = nbody::init_state(cnf.variable_count(), params, 42);
    auto deriv = nbody::build_deriv(cnf, params);

    state_t dst;
    deriv(state, dst);

    std::cout << "variable_count = " << cnf.variable_count() << "\n";
    std::cout << "clause_count   = " << cnf.clause_count() << "\n";
    std::cout << "state.size()   = " << state.size() << "\n";
    std::cout << "dst.size()     = " << dst.size() << "\n";

    bool ok = true;
    for (double x : dst) {
        if (!std::isfinite(x)) {
            ok = false;
            break;
        }
    }

    std::cout << "derivative finite: " << (ok ? "yes" : "no") << "\n";

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "first values of derivative:\n";
    for (std::size_t i = 0; i < std::min<std::size_t>(dst.size(), 20); ++i) {
        std::cout << dst[i] << ' ';
    }
    std::cout << "\n";

    auto decoded = nbody::decode_best(cnf, state, params);
    if (decoded.decoded) {
        std::cout
            << "initial decoded satisfied = "
            << decoded.satisfied
            << " / "
            << cnf.clause_count()
            << "\n";
    } else {
        std::cout << "initial state was not decoded\n";
    }

    return ok ? 0 : 2;
}
