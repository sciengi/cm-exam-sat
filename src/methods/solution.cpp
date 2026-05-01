#include <methods/solution.hpp>

#include <random>
#include <cmath>

// TODO: localize state size



state_t init_state(size_t variable_count) {

    state_t state(2 * variable_count);

    std::random_device rd; 
    std::default_random_engine gen(rd());
    std::uniform_real_distribution distr;

    for (auto& c : state) { c = distr(gen); }

    return state;

    // TODO: add seed
}


deriv_t build_deriv(const CNF& cnf, double alpha, double beta, double lambda, double mu) {

    const size_t L = cnf.variable_count();
    const size_t N = cnf.clause_count();

    std::vector<double> pairs(L);  // DEV: in this method all system members scalars
    std::vector<double> triples(N);

    std::vector<std::vector<size_t>> cl_indexies(2 * L);
    for (size_t i = 0; i < cl_indexies.size(); i++) {
        // add ...
    }

    return [
        &cnf,
        alpha, beta, 
        lambda, mu,
        L, N,
        pairs   = std::move(pairs),
        triples = std::move(triples),
        cl_indexies = std::move(cl_indexies)
    ](const state_t& x, state_t& dx) mutable {

        for (size_t i = 0; i < L; i++) {
            pairs[i] = -alpha * std::pow(x[i] * x[i + L], lambda);
        }

        double cl_mul;
        for (size_t i = 0; i < N; i++) {
           
            int q1 = cnf[i][0] > 0 ? cnf[i][0] : -cnf[i][0] + L; // TODO: add to CNF
            int q2 = cnf[i][1] > 0 ? cnf[i][1] : -cnf[i][1] + L;
            int q3 = cnf[i][2] > 0 ? cnf[i][2] : -cnf[i][2] + L;

            cl_mul = x[q1] * x[q2] * x[q3];

            triples[i] = std::pow(cl_mul, mu);
        }

        double cl_sum;
        for (size_t i = 0; i < dx.size(); i++) {
            cl_sum = 0;
            for (auto& ind : cl_indexies[i]) { cl_sum += triples[ind]; }

            dx[i] = pairs[i % L] - beta * cl_sum;
        }


    };

    // TODO: clip to [0, inf)
}


void decode(const state_t& state, CNF::model& model) {
    size_t offset = state.size() / 2;
    for (size_t i = 0; i < model.size(); i++) {
        model[i] = state[i] < state[i + offset];
    }

    // TODO: float-comparing, add threshold?
}

