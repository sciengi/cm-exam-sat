#include <methods/canonical_nbody.hpp>
#include <random>
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace canonical_nbody {

state_t init_canonical_state(const CNF& cnf, const CanonicalParams& params, unsigned seed) {
    size_t total_atoms = 3 * cnf.clause_count();
    state_t state(2 * total_atoms * params.dim, 0.0);

    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> pos_dist(-1.0, 1.0);
    std::uniform_real_distribution<double> vel_dist(-0.05, 0.05);

    for (size_t i = 0; i < state.size(); ++i) {
        if (i < total_atoms * params.dim) {
            state[i] = pos_dist(gen);
        } else {
            state[i] = vel_dist(gen);
        }
    }
    return state;
}

deriv_t build_canonical_deriv(const CNF& cnf, const CanonicalParams& params) {
    size_t C = cnf.clause_count();
    size_t total_atoms = 3 * C;
    size_t dim = params.dim;
    double eps2 = params.eps * params.eps;

    // Предвычисляем карту литералов для каждого из 3C атомов
    std::vector<CanonicalAtom> atoms_map(total_atoms);
    for (size_t m = 0; m < C; ++m) {
        auto clause = cnf[static_cast<int>(m)];
        for (size_t q = 0; q < 3; ++q) {
            atoms_map[m * 3 + q] = { m, q, clause[q] };
        }
    }

    return [total_atoms, dim, eps2, params, atoms_map, C](const state_t& state, state_t& dst) -> void {
        dst.assign(state.size(), 0.0);

        // 1. Координаты изменяются по скоростям: r' = v
        for (size_t i = 0; i < total_atoms * dim; ++i) {
            dst[i] = state[total_atoms * dim + i];
        }

        std::vector<double> forces(total_atoms * dim, 0.0);

        // 2. Взаимодействие МЕЖДУ атомами разных дизъюнктов (Законы 1 и 2)
        for (size_t i = 0; i < total_atoms; ++i) {
            int lit_i = atoms_map[i].literal;
            
            for (size_t j = i + 1; j < total_atoms; ++j) {
                int lit_j = atoms_map[j].literal;

                // Считаем расстояние между телом i и j
                double norm2 = eps2;
                for (size_t d = 0; d < dim; ++d) {
                    double delta = state[i * dim + d] - state[j * dim + d];
                    norm2 += delta * delta;
                }
                double rho3 = std::pow(norm2, 1.5);

                double coeff = 0.0;
                if (lit_i == lit_j) {
                    coeff = -params.c_att; // Закон 1: Притяжение одинаковых литералов
                } else if (lit_i == -lit_j) {
                    coeff = params.c_opp;   // Закон 2: Отталкивание конфликтующих
                }

                if (std::abs(coeff) > 1e-9) {
                    for (size_t d = 0; d < dim; ++d) {
                        double delta = state[i * dim + d] - state[j * dim + d];
                        double f = coeff * delta / rho3;
                        forces[i * dim + d] += f;
                        forces[j * dim + d] -= f;
                    }
                }
            }
        }

        // 3. Расталкивание внутри скобок от общего центра (Закон 3)
        for (size_t m = 0; m < C; ++m) {
            std::vector<double> center(dim, 0.0);
            for (size_t q = 0; q < 3; ++q) {
                for (size_t d = 0; d < dim; ++d) {
                    center[d] += state[(m * 3 + q) * dim + d] / 3.0;
                }
            }

            for (size_t q = 0; q < 3; ++q) {
                size_t atom_idx = m * 3 + q;
                double norm2 = eps2;
                for (size_t d = 0; d < dim; ++d) {
                    double u = state[atom_idx * dim + d] - center[d];
                    norm2 += u * u;
                }
                double rho3 = std::pow(norm2, 1.5);

                for (size_t d = 0; d < dim; ++d) {
                    double u = state[atom_idx * dim + d] - center[d];
                    forces[atom_idx * dim + d] += params.c_clause * u / rho3;
                }
            }
        }

        // 4. Запись ускорений с учетом линейного трения: v' = force - gamma * v
        for (size_t i = 0; i < total_atoms * dim; ++i) {
            double v = state[total_atoms * dim + i];
            dst[total_atoms * dim + i] = forces[i] - params.gamma * v;
        }
    };
}

size_t decode_canonical_majority(const CNF& cnf, const state_t& state, const CanonicalParams& params, CNF::model& out_model) {
    size_t L = cnf.variable_count();
    size_t C = cnf.clause_count();
    size_t total_atoms = 3 * C;
    size_t dim = params.dim;

    out_model.assign(L, false);

    // Собираем проекции координат по первой оси (X) для каждого литерала
    std::vector<std::vector<double>> pos_lits(L + 1);
    std::vector<std::vector<double>> neg_lits(L + 1);

    for (size_t i = 0; i < total_atoms; ++i) {
        size_t m = i / 3;
        size_t q = i % 3;
        int lit = cnf[static_cast<int>(m)][q];
        double x_coord = state[i * dim + 0]; // Проекция на ось X

        if (lit > 0) {
            pos_lits[static_cast<size_t>(lit)].push_back(x_coord);
        } else {
            neg_lits[static_cast<size_t>(-lit)].push_back(x_coord);
        }
    }

    // Мажоритарное голосование: сравниваем средние координаты центров масс клонов
    for (size_t v = 1; v <= L; ++v) {
        double pos_sum = 0.0, neg_sum = 0.0;
        for (double x : pos_lits[v]) pos_sum += x;
        for (double x : neg_lits[v]) neg_sum += x;

        double pos_mean = pos_lits[v].empty() ? 0.0 : pos_sum / pos_lits[v].size();
        double neg_mean = neg_lits[v].empty() ? 0.0 : neg_sum / neg_lits[v].size();

        out_model[v - 1] = (pos_mean > neg_mean);
    }

    // Считаем качество полученной дискретной модели
    size_t satisfied = 0;
    for (size_t m = 0; m < C; ++m) {
        auto clause = cnf[static_cast<int>(m)];
        bool clause_ok = false;
        for (size_t q = 0; q < 3; ++q) {
            int lit = clause[q];
            bool val = out_model[static_cast<size_t>(std::abs(lit) - 1)];
            if ((lit > 0 && val) || (lit < 0 && !val)) {
                clause_ok = true;
                break;
            }
        }
        if (clause_ok) satisfied++;
    }
    return satisfied;
}

} // namespace canonical_nbody