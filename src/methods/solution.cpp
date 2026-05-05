#include <methods/solution.hpp>

#include <random>
#include <cmath>

// TODO: localize state size



static size_t literal_to_state_index(int lit, std::size_t L) {
    if (lit > 0) {
        return static_cast<size_t>(lit - 1);
    }

    return static_cast<size_t>(-lit - 1) + L;
}


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

    // DEV: 
    // - DIMACS variables has 1-based index with negative indexies for NEG
    // - method array is 0-based index with + L shift for NEG

    std::vector<std::vector<size_t>> cl_indexies(2 * L);
    for (size_t i = 0; i < cnf.clause_count(); i++) {
        for (size_t q = 0; q < cnf.var_in_clause; q++) {
            size_t ind = cnf[i][q] > 0 ? cnf[i][q] - 1 : -cnf[i][q] + L - 1;
            cl_indexies[ind].push_back(i);  
        }
    }
    
    // DEV: suppose (okey for DIMACS) that no mixed cnf => one concrete literal in clause 

    return [
        &cnf,
        alpha, beta, 
        lambda, mu,
        L, N,
        pairs   = std::move(pairs),
        triples = std::move(triples),
        cl_indexies = std::move(cl_indexies)
    ](const state_t& x, state_t& dx) mutable -> void {

        for (size_t i = 0; i < L; i++) {
            pairs[i] = -alpha * std::pow(x[i] * x[i + L], lambda);
        }

        double cl_mul;
        for (size_t i = 0; i < N; i++) {
           
            size_t q1 = literal_to_state_index(cnf[i][0], L);
            size_t q2 = literal_to_state_index(cnf[i][1], L);
            size_t q3 = literal_to_state_index(cnf[i][2], L);

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

