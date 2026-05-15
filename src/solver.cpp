#include <cnf/cnf.hpp>
#include <methods/solution.hpp>
#include <numeric/ode.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

static void print_model(const CNF::model& model)
{
    for (bool value : model) {
        std::cout << (value ? "1" : "0") << " ";
    }
    std::cout << "\n";
}

static std::size_t count_satisfied_clauses(const CNF& cnf, const CNF::model& model)
{
    std::size_t satisfied = 0;

    for (std::size_t i = 0; i < cnf.clause_count(); ++i) {
        auto clause = cnf[i];

        bool clause_ok = false;

        for (std::size_t j = 0; j < CNF::var_in_clause; ++j) {
            int lit = clause[j];

            bool lit_value = lit > 0
                ? model[static_cast<std::size_t>(lit - 1)]
                : !model[static_cast<std::size_t>(-lit - 1)];

            clause_ok = clause_ok || lit_value;
        }

        if (clause_ok) {
            ++satisfied;
        }
    }

    return satisfied;
}

int main()
{
    std::string dimacs_path;
    double alpha;
    double beta;
    double lambda;
    double mu;

    if (!(std::cin >> dimacs_path >> alpha >> beta >> lambda >> mu)) {
        std::cerr << "Usage: <dimacs_path> alpha beta lambda mu\n";
        return 1;
    }

    CNF cnf(dimacs_path);

    auto state = init_state(cnf.variable_count());
    auto deriv = build_deriv(cnf, alpha, beta, lambda, mu);

    const double initial_step = 1e-3;
    const std::size_t max_steps = 100000;
    const std::size_t log_every = 1000;

    some_ode_solver nm(state.size(), initial_step, OdeMethod::RK4);

    double t = 0.0;

    CNF::model model(cnf.variable_count());
    decode(state, model);

    std::size_t best_satisfied = count_satisfied_clauses(cnf, model);
    CNF::model best_model = model;

    for (std::size_t step = 0; step < max_steps; ++step) {
        if (cnf(model)) {
            std::cout << "SAT found at step " << step << ", t = " << t << "\n";
            std::cout << "Model: ";
            print_model(model);
            return 0;
        }

        nm.step(state, t, deriv);

        // Для метода I концентрации не должны уходить в отрицательные значения.
        for (double& x : state) {
            if (!std::isfinite(x)) {
                std::cerr << "FAIL: state contains nan/inf at step " << step << "\n";
                return 2;
            }

            if (x < 0.0) {
                x = 0.0;
            }
        }

        decode(state, model);

        std::size_t satisfied = count_satisfied_clauses(cnf, model);

        if (satisfied > best_satisfied) {
            best_satisfied = satisfied;
            best_model = model;
        }

        if (step % log_every == 0) {
            std::cout
                << "step = " << step
                << ", t = " << t
                << ", satisfied = " << satisfied
                << " / " << cnf.clause_count()
                << "\n";
        }
    }

    std::cout << "FAIL: max steps reached\n";
    std::cout << "Best satisfied: " << best_satisfied << " / " << cnf.clause_count() << "\n";
    std::cout << "Best model: ";
    print_model(best_model);

    return 1;
}