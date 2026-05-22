#include <methods/nbody.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nbody {
namespace {

std::size_t pos_index(std::size_t atom,
                      std::size_t coord,
                      std::size_t dim)
{
    return atom * dim + coord;
}

std::size_t vel_index(std::size_t atom,
                      std::size_t coord,
                      std::size_t atoms,
                      std::size_t dim)
{
    return atoms * dim + atom * dim + coord;
}

double sqr(double x)
{
    return x * x;
}

double squared_distance_to_atom(const state_t& state,
                                std::size_t a,
                                std::size_t b,
                                std::size_t dim)
{
    double result = 0.0;

    for (std::size_t d = 0; d < dim; ++d) {
        const double diff = state[pos_index(a, d, dim)] - state[pos_index(b, d, dim)];
        result += diff * diff;
    }

    return result;
}

std::vector<std::array<std::size_t, 3>> build_clause_atom_types(const CNF& cnf)
{
    const std::size_t L = cnf.variable_count();

    std::vector<std::array<std::size_t, 3>> clauses;
    clauses.reserve(cnf.clause_count());

    for (std::size_t i = 0; i < cnf.clause_count(); ++i) {
        const auto clause = cnf[static_cast<int>(i)];

        clauses.push_back({
            literal_to_atom_type(clause[0], L),
            literal_to_atom_type(clause[1], L),
            literal_to_atom_type(clause[2], L)
        });
    }

    return clauses;
}

bool build_model_from_M(const std::vector<bool>& in_M,
                        std::size_t L,
                        CNF::model& model)
{
    model.assign(L, false);

    for (std::size_t i = 0; i < L; ++i) {
        const bool pos_false = in_M[i];
        const bool neg_false = in_M[i + L];

        // Должно быть ровно одно из двух:
        // a_i     in M  => x_i = false
        // a_{i+L} in M  => not x_i = false => x_i = true
        if (pos_false == neg_false) {
            return false;
        }

        model[i] = neg_false;
    }

    return true;
}

} // namespace

std::size_t atom_count(std::size_t variable_count)
{
    return 2 * variable_count;
}

std::size_t state_size(std::size_t variable_count, std::size_t dim)
{
    return 2 * atom_count(variable_count) * dim;
}

std::size_t literal_to_atom_type(int lit, std::size_t variable_count)
{
    if (lit == 0) {
        throw std::runtime_error("Zero literal cannot be converted to atom type");
    }

    if (lit > 0) {
        return static_cast<std::size_t>(lit - 1);
    }

    return static_cast<std::size_t>(-lit - 1) + variable_count;
}

std::size_t opposite_type(std::size_t atom_type, std::size_t variable_count)
{
    return (atom_type + variable_count) % (2 * variable_count);
}

state_t init_state(std::size_t variable_count,
                   const NBodyParams& params,
                   unsigned seed)
{
    if (params.dim == 0) {
        throw std::runtime_error("NBody: dim must be positive");
    }

    const std::size_t atoms = atom_count(variable_count);
    state_t state(2 * atoms * params.dim, 0.0);

    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> pos_dist(
        -params.initial_position_scale,
        params.initial_position_scale
    );
    std::uniform_real_distribution<double> vel_dist(
        -params.initial_velocity_scale,
        params.initial_velocity_scale
    );

    for (std::size_t i = 0; i < atoms; ++i) {
        for (std::size_t d = 0; d < params.dim; ++d) {
            state[pos_index(i, d, params.dim)] = pos_dist(gen);
            state[vel_index(i, d, atoms, params.dim)] = vel_dist(gen);
        }
    }

    return state;
}

deriv_t build_deriv(const CNF& cnf,
                    const NBodyParams& params)
{
    const std::size_t L = cnf.variable_count();
    const std::size_t atoms = atom_count(L);
    const std::size_t dim = params.dim;
    const double eps2 = params.eps * params.eps;

    const auto clauses = build_clause_atom_types(cnf);

    return [L, atoms, dim, eps2, params, clauses](const state_t& state,
                                                   state_t& dst) -> void {
        if (state.size() != 2 * atoms * dim) {
            throw std::runtime_error("NBody deriv: wrong state size");
        }

        dst.assign(state.size(), 0.0);

        // r' = v
        for (std::size_t i = 0; i < atoms; ++i) {
            for (std::size_t d = 0; d < dim; ++d) {
                dst[pos_index(i, d, dim)] = state[vel_index(i, d, atoms, dim)];
            }
        }

        // force хранит ускорения, так как массы пока считаем равными 1.
        std::vector<double> force(atoms * dim, 0.0);

        // 1) Парные силы:
        // - противоположные литералы отталкиваются;
        // - остальные непарные атомы притягиваются.
        for (std::size_t i = 0; i < atoms; ++i) {
            for (std::size_t j = i + 1; j < atoms; ++j) {
                double norm2 = eps2;

                for (std::size_t d = 0; d < dim; ++d) {
                    const double delta =
                        state[pos_index(i, d, dim)] - state[pos_index(j, d, dim)];
                    norm2 += delta * delta;
                }

                const double rho3 = std::pow(norm2, 1.5);

                const bool opposite = (j == opposite_type(i, L));
                const double coeff = opposite ? params.c_opp : -params.c_att;

                for (std::size_t d = 0; d < dim; ++d) {
                    const double delta =
                        state[pos_index(i, d, dim)] - state[pos_index(j, d, dim)];

                    const double f = coeff * delta / rho3;

                    force[pos_index(i, d, dim)] += f;
                    force[pos_index(j, d, dim)] -= f;
                }
            }
        }

        // 2) Расталкивание троек из дизъюнктов.
        // Для каждого дизъюнкта берём центр трёх атомов и толкаем каждый атом от центра.
        for (const auto& clause : clauses) {
            std::array<double, 3> center{};

            for (std::size_t d = 0; d < dim; ++d) {
                center[d] =
                    (
                        state[pos_index(clause[0], d, dim)] +
                        state[pos_index(clause[1], d, dim)] +
                        state[pos_index(clause[2], d, dim)]
                    ) / 3.0;
            }

            for (std::size_t q = 0; q < 3; ++q) {
                const std::size_t atom = clause[q];

                double norm2 = eps2;

                for (std::size_t d = 0; d < dim; ++d) {
                    const double u = state[pos_index(atom, d, dim)] - center[d];
                    norm2 += u * u;
                }

                const double rho3 = std::pow(norm2, 1.5);

                for (std::size_t d = 0; d < dim; ++d) {
                    const double u = state[pos_index(atom, d, dim)] - center[d];
                    force[pos_index(atom, d, dim)] += params.c_clause * u / rho3;
                }
            }
        }

        // 3) v' = force - gamma * v
        for (std::size_t i = 0; i < atoms; ++i) {
            for (std::size_t d = 0; d < dim; ++d) {
                const double v = state[vel_index(i, d, atoms, dim)];
                dst[vel_index(i, d, atoms, dim)] =
                    force[pos_index(i, d, dim)] - params.gamma * v;
            }
        }
    };
}

std::size_t count_satisfied_clauses(const CNF& cnf,
                                    const CNF::model& model)
{
    std::size_t satisfied = 0;

    for (std::size_t i = 0; i < cnf.clause_count(); ++i) {
        const auto clause = cnf[static_cast<int>(i)];

        bool clause_ok = false;

        for (std::size_t q = 0; q < CNF::var_in_clause; ++q) {
            const int lit = clause[q];

            const bool lit_value = lit > 0
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

DecodeResult decode_best(const CNF& cnf,
                         const state_t& state,
                         const NBodyParams& params)
{
    const std::size_t L = cnf.variable_count();
    const std::size_t atoms = atom_count(L);
    const std::size_t dim = params.dim;

    if (state.size() != 2 * atoms * dim) {
        throw std::runtime_error("NBody decode: wrong state size");
    }

    DecodeResult best;
    best.model.assign(L, false);

    for (std::size_t start = 0; start < atoms; ++start) {
        std::vector<std::pair<double, std::size_t>> order;
        order.reserve(atoms);

        for (std::size_t atom = 0; atom < atoms; ++atom) {
            const double dist2 = squared_distance_to_atom(state, start, atom, dim);
            order.push_back({dist2, atom});
        }

        std::sort(order.begin(), order.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return lhs.first < rhs.first;
                  });

        std::vector<bool> in_M(atoms, false);
        std::size_t selected = 0;

        for (const auto& [dist2, atom] : order) {
            (void)dist2;

            if (in_M[atom]) {
                continue;
            }

            const std::size_t opp = opposite_type(atom, L);

            if (in_M[opp]) {
                continue;
            }

            in_M[atom] = true;
            ++selected;

            if (selected == L) {
                break;
            }
        }

        if (selected != L) {
            continue;
        }

        CNF::model candidate;
        if (!build_model_from_M(in_M, L, candidate)) {
            continue;
        }

        const std::size_t satisfied = count_satisfied_clauses(cnf, candidate);

        if (!best.decoded || satisfied > best.satisfied) {
            best.decoded = true;
            best.satisfied = satisfied;
            best.sat = (satisfied == cnf.clause_count());
            best.start_atom = start;
            best.model = candidate;
        }

        if (best.sat) {
            return best;
        }
    }

    return best;
}

bool decode(const CNF& cnf,
            const state_t& state,
            const NBodyParams& params,
            CNF::model& model)
{
    DecodeResult result = decode_best(cnf, state, params);

    if (!result.decoded) {
        return false;
    }

    model = result.model;
    return true;
}

} // namespace nbody
