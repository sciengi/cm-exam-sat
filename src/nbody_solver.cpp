#include <cnf/cnf.hpp>
#include <methods/nbody.hpp>
#include <numeric/ode.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

void print_model(const CNF::model& model)
{
    for (bool value : model) {
        std::cout << (value ? "1" : "0") << ' ';
    }
    std::cout << '\n';
}

const char* method_name(OdeMethod method)
{
    switch (method) {
    case OdeMethod::Euler: return "Euler";
    case OdeMethod::Leapfrog: return "Leapfrog";
    case OdeMethod::RK4: return "RK4";
    case OdeMethod::DP8: return "DP8";
    case OdeMethod::DP8Adaptive: return "DP8Adaptive";
    }

    return "Unknown";
}

void print_unsatisfied_clauses(const CNF& cnf, const CNF::model& model)
{
    for (std::size_t i = 0; i < cnf.clause_count(); ++i) {
        auto clause = cnf[i];

        bool ok = false;

        for (std::size_t j = 0; j < CNF::var_in_clause; ++j) {
            int lit = clause[j];

            bool value = lit > 0
                ? model[static_cast<std::size_t>(lit - 1)]
                : !model[static_cast<std::size_t>(-lit - 1)];

            ok = ok || value;
        }

        if (!ok) {
            std::cout << "Unsatisfied clause #" << i << ": "
                      << clause[0] << " "
                      << clause[1] << " "
                      << clause[2] << "\n";
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2 && argc != 5) {
        std::cerr
            << "Usage:\n"
            << "  nbody_solver <dimacs_path>\n"
            << "  nbody_solver <dimacs_path> <dim> <max_steps> <restarts>\n";
        return 1;
    }

    const std::string dimacs_path = argv[1];

    std::size_t dim = 3;
    std::size_t max_steps = 5000;
    std::size_t restarts = 3;

    if (argc == 5) {
        dim = static_cast<std::size_t>(std::stoul(argv[2]));
        max_steps = static_cast<std::size_t>(std::stoul(argv[3]));
        restarts = static_cast<std::size_t>(std::stoul(argv[4]));
    }

    CNF cnf(dimacs_path);

    const std::vector<double> c_opp_values    = {0.5, 1.0, 2.0};
    const std::vector<double> c_att_values    = {0.01, 0.05};
    const std::vector<double> c_clause_values = {0.5, 1.0, 2.0};
    const std::vector<double> gamma_values    = {0.05, 0.2};

    const double step_size = 1e-3;
    const std::size_t decode_every = 100;

    const OdeMethod method = OdeMethod::RK4;

    nbody::DecodeResult global_best;
    global_best.model.assign(cnf.variable_count(), false);

    std::size_t run_id = 0;

    for (double c_opp : c_opp_values) {
        for (double c_att : c_att_values) {
            for (double c_clause : c_clause_values) {
                for (double gamma : gamma_values) {
                    for (std::size_t restart = 0; restart < restarts; ++restart) {
                        nbody::NBodyParams params;
                        params.dim = dim;
                        params.c_opp = c_opp;
                        params.c_att = c_att;
                        params.c_clause = c_clause;
                        params.gamma = gamma;

                        const unsigned seed = static_cast<unsigned>(1234 + 1009 * run_id + restart);

                        auto state = nbody::init_state(cnf.variable_count(), params, seed);
                        auto deriv = nbody::build_deriv(cnf, params);

                        some_ode_solver solver(state.size(), step_size, method);

                        double t = 0.0;

                        nbody::DecodeResult run_best;
                        run_best.model.assign(cnf.variable_count(), false);

                        for (std::size_t step = 0; step < max_steps; ++step) {
                            solver.step(state, t, deriv);

                            bool bad_state = false;
                            for (double x : state) {
                                if (!std::isfinite(x)) {
                                    bad_state = true;
                                    break;
                                }
                            }

                            if (bad_state) {
                                std::cout
                                    << "run = " << run_id
                                    << " failed: nan/inf\n";
                                break;
                            }

                            if (step % decode_every != 0) {
                                continue;
                            }

                            auto decoded = nbody::decode_best(cnf, state, params);

                            if (!decoded.decoded) {
                                continue;
                            }

                            if (!run_best.decoded || decoded.satisfied > run_best.satisfied) {
                                run_best = decoded;
                            }

                            if (!global_best.decoded || decoded.satisfied > global_best.satisfied) {
                                global_best = decoded;

                                std::cout
                                    << "NEW BEST: "
                                    << global_best.satisfied << " / " << cnf.clause_count()
                                    << ", run = " << run_id
                                    << ", step = " << step
                                    << ", t = " << t
                                    << ", method = " << method_name(method)
                                    << ", opp = " << c_opp
                                    << ", att = " << c_att
                                    << ", clause = " << c_clause
                                    << ", gamma = " << gamma
                                    << ", restart = " << restart
                                    << "\n";
                            }

                            if (decoded.sat) {
                                std::cout << "SAT found!\n";
                                std::cout << "Model: ";
                                print_model(decoded.model);
                                return 0;
                            }
                        }

                        std::cout
                            << "run = " << run_id
                            << ", best = "
                            << (run_best.decoded ? run_best.satisfied : 0)
                            << " / " << cnf.clause_count()
                            << ", opp = " << c_opp
                            << ", att = " << c_att
                            << ", clause = " << c_clause
                            << ", gamma = " << gamma
                            << ", restart = " << restart
                            << "\n";

                        ++run_id;
                    }
                }
            }
        }
    }

    std::cout << "FAIL: parameter sweep finished\n";

    if (global_best.decoded) {
        std::cout
            << "Global best: "
            << global_best.satisfied
            << " / " << cnf.clause_count()
            << "\n";
        std::cout << "Best model: ";
        print_model(global_best.model);
        print_unsatisfied_clauses(cnf, global_best.model);
    } else {
        std::cout << "No decoded model was produced\n";
    }

    return global_best.sat ? 0 : 1;
}
