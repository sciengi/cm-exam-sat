#include <methods/nbody.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace nbody {
namespace {

std::size_t atom_count(std::size_t variable_count) { return 2 * variable_count; }
std::size_t pos_index(std::size_t atom, std::size_t coord, std::size_t dim) { return atom * dim + coord; }
std::size_t vel_index(std::size_t atom, std::size_t coord, std::size_t atoms, std::size_t dim) {
    return atoms * dim + atom * dim + coord;
}
double sqr(double x) { return x * x; }

double squared_distance_to_atom(const state_t& state, std::size_t a, std::size_t b, std::size_t dim) {
    double result = 0.0;
    for (std::size_t d = 0; d < dim; ++d) {
        const double diff = state[pos_index(a, d, dim)] - state[pos_index(b, d, dim)];
        result += diff * diff;
    }
    return result;
}

std::vector<std::array<std::size_t, 3>> build_clause_atom_types(const CNF& cnf) {
    const std::size_t L = cnf.variable_count();
    std::vector<std::array<std::size_t, 3>> result;
    result.reserve(cnf.clause_count());
    for (int m = 0; m < cnf.clause_count(); ++m) {
        auto clause = cnf[m];
        result.push_back({
            literal_to_atom_type(clause[0], L),
            literal_to_atom_type(clause[1], L),
            literal_to_atom_type(clause[2], L)
        });
    }
    return result;
}

bool build_model_from_M(const std::vector<bool>& in_M, std::size_t L, CNF::model& model) {
    model.assign(L, false);
    for (std::size_t v = 0; v < L; ++v) {
        const bool has_pos = in_M[v];
        const bool has_neg = in_M[v + L];
        if (has_pos && has_neg) return false; 
        if (has_pos) model[v] = true;
        else if (has_neg) model[v] = false;
    }
    return true;
}

} // namespace

std::size_t state_size(std::size_t variable_count, std::size_t dim) { return 2 * atom_count(variable_count) * dim; }
std::size_t literal_to_atom_type(int lit, std::size_t variable_count) {
    if (lit > 0) return static_cast<std::size_t>(lit - 1);
    return static_cast<std::size_t>(variable_count + (-lit - 1));
}
std::size_t opposite_type(std::size_t atom_type, std::size_t variable_count) {
    if (atom_type < variable_count) return atom_type + variable_count;
    return atom_type - variable_count;
}

state_t init_state(std::size_t variable_count, const NBodyParams& params, unsigned seed) {
    const std::size_t atoms = atom_count(variable_count);
    state_t state(state_size(variable_count, params.dim), 0.0);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> pos_dist(-params.initial_position_scale, params.initial_position_scale);
    std::uniform_real_distribution<double> vel_dist(-params.initial_velocity_scale, params.initial_velocity_scale);

    for (std::size_t atom = 0; atom < atoms; ++atom) {
        for (std::size_t d = 0; d < params.dim; ++d) {
            state[pos_index(atom, d, params.dim)] = pos_dist(gen);
            state[vel_index(atom, d, atoms, params.dim)] = vel_dist(gen);
        }
    }
    return state;
}

deriv_t build_deriv(const CNF& cnf, const NBodyParams& params) {
    const std::size_t L = cnf.variable_count();
    const std::size_t atoms = atom_count(L);
    const std::size_t dim = params.dim;
    const double eps2 = sqr(params.eps);
    auto clauses_cache = build_clause_atom_types(cnf);

    return [L, atoms, dim, eps2, params, clauses_cache](const state_t& state, state_t& dst) -> void {
        dst.assign(state.size(), 0.0);
        for (std::size_t i = 0; i < atoms * dim; ++i) dst[i] = state[atoms * dim + i];
        std::vector<double> forces(atoms * dim, 0.0);

        for (std::size_t i = 0; i < atoms; ++i) {
            const std::size_t opp_i = opposite_type(i, L);
            for (std::size_t j = i + 1; j < atoms; ++j) {
                double dist2 = eps2;
                for (std::size_t d = 0; d < dim; ++d) {
                    dist2 += sqr(state[pos_index(i, d, dim)] - state[pos_index(j, d, dim)]);
                }
                const double rho3 = std::pow(dist2, 1.5);
                double coeff = (j == opp_i) ? params.c_opp : -params.c_att;

                for (std::size_t d = 0; d < dim; ++d) {
                    double delta = state[pos_index(i, d, dim)] - state[pos_index(j, d, dim)];
                    double f = coeff * delta / rho3;
                    forces[i * dim + d] += f;
                    forces[j * dim + d] -= f;
                }
            }
        }

        for (const auto& clause_atoms : clauses_cache) {
            std::array<double, 3> center = {0.0, 0.0, 0.0};
            for (std::size_t d = 0; d < dim; ++d) {
                center[d] = (state[pos_index(clause_atoms[0], d, dim)] +
                             state[pos_index(clause_atoms[1], d, dim)] +
                             state[pos_index(clause_atoms[2], d, dim)]) / 3.0;
            }
            for (std::size_t q = 0; q < 3; ++q) {
                const std::size_t atom = clause_atoms[q];
                double dist2 = eps2;
                for (std::size_t d = 0; d < dim; ++d) dist2 += sqr(state[pos_index(atom, d, dim)] - center[d]);
                const double rho3 = std::pow(dist2, 1.5);
                for (std::size_t d = 0; d < dim; ++d) {
                    double delta = state[pos_index(atom, d, dim)] - center[d];
                    forces[atom * dim + d] += params.c_clause * delta / rho3;
                }
            }
        }

        for (std::size_t atom = 0; atom < atoms; ++atom) {
            for (std::size_t d = 0; d < dim; ++d) {
                const std::size_t v_idx = vel_index(atom, d, atoms, dim);
                dst[v_idx] = forces[atom * dim + d] - params.gamma * state[v_idx];
            }
        }
    };
}

std::size_t count_satisfied_clauses(const CNF& cnf, const CNF::model& model) {
    std::size_t result = 0;
    for (int m = 0; m < cnf.clause_count(); ++m) {
        auto clause = cnf[m];
        bool clause_satisfied = false;
        for (int i = 0; i < 3; ++i) {
            int lit = clause[i];
            bool val = model[static_cast<std::size_t>(std::abs(lit) - 1)];
            if ((lit > 0 && val) || (lit < 0 && !val)) { clause_satisfied = true; break; }
        }
        if (clause_satisfied) ++result;
    }
    return result;
}

DecodeResult decode_best(const CNF& cnf, const state_t& state, const NBodyParams& params) {
    const std::size_t L = cnf.variable_count();
    const std::size_t atoms = atom_count(L);
    const std::size_t dim = params.dim;

    DecodeResult best;
    best.decoded = false;
    best.satisfied = 0;
    best.sat = false;

    std::vector<std::pair<double, std::size_t>> order(atoms);

    for (std::size_t start = 0; start < atoms; ++start) {
        for (std::size_t i = 0; i < atoms; ++i) {
            order[i] = {squared_distance_to_atom(state, start, i, dim), i};
        }

        std::sort(order.begin(), order.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

        std::vector<bool> in_M(atoms, false);
        std::size_t selected = 0;
        bool conflict_broken = false;

        for (const auto& [dist2, atom] : order) {
            (void)dist2;
            if (in_M[atom]) continue;

            const std::size_t opp = opposite_type(atom, L);

            if (in_M[opp]) {
                // ВЕТВЛЕНИЕ ПО СТРАТЕГИЯМ ДЕКОДИРОВАНИЯ
                if (params.strategy == DecodeStrategy::HardStop) {
                    conflict_broken = true;
                    break; // Наша новая идея: жесткий Break
                } else {
                    continue; // Старая стратегия из книги: мягкий Continue
                }
            }

            in_M[atom] = true;
            ++selected;

            if (selected == L) break;
        }

        if (conflict_broken || selected != L) continue; 

        CNF::model candidate;
        if (!build_model_from_M(in_M, L, candidate)) continue;

        const std::size_t satisfied = count_satisfied_clauses(cnf, candidate);

        if (!best.decoded || satisfied > best.satisfied) {
            best.decoded = true;
            best.satisfied = satisfied;
            best.sat = (satisfied == cnf.clause_count());
            best.start_atom = start;
            best.model = candidate;
        }

        if (best.sat) return best;
    }
    return best;
}

} // namespace nbody