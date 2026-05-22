#include <numeric/ode.hpp>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <vector>

namespace {

struct Term {
    double coeff;
    const state_t* k;
};

void add_scaled(state_t& y, const state_t& k, double scale)
{
    for (std::size_t i = 0; i < y.size(); ++i) {
        y[i] += scale * k[i];
    }
}

state_t make_stage_state(const state_t& y, double h, std::initializer_list<Term> terms)
{
    state_t tmp = y;

    for (const Term& term : terms) {
        const state_t& k = *term.k;
        for (std::size_t i = 0; i < y.size(); ++i) {
            tmp[i] += h * term.coeff * k[i];
        }
    }

    return tmp;
}

// Коэффициенты DOP853 / Dormand-Prince 8-го порядка.
static constexpr double a21 =  5.26001519587677318e-02;

static constexpr double a31 =  1.97250569845378994e-02;
static constexpr double a32 =  5.91751709536136983e-02;

static constexpr double a41 =  2.95875854768068491e-02;
static constexpr double a43 =  8.87627564304205475e-02;

static constexpr double a51 =  2.41365641954049e-01;
static constexpr double a53 = -8.84549479328286086e-01;
static constexpr double a54 =  9.24834003261792003e-01;

static constexpr double a61 =  3.7037037037037037e-02;
static constexpr double a64 =  1.70828608729473871e-01;
static constexpr double a65 =  1.25467687566822429e-01;

static constexpr double a71 =  3.7109375e-02;
static constexpr double a74 =  1.70252211019544039e-01;
static constexpr double a75 =  6.02165389804559092e-02;
static constexpr double a76 = -1.7578125e-02;

static constexpr double a81 =  3.70920001185047927e-02;
static constexpr double a84 =  1.70383925712239993e-01;
static constexpr double a85 =  1.07262030446373284e-01;
static constexpr double a86 = -1.53194377486244882e-02;
static constexpr double a87 =  8.27378916792996988e-03;

static constexpr double a91 =  6.24110958716075712e-01;
static constexpr double a94 = -3.36089262944694129e+00;
static constexpr double a95 = -8.68219346841726006e-01;
static constexpr double a96 =  2.72170569800576667e+01;
static constexpr double a97 = -2.43093619143939525e+01;
static constexpr double a98 =  7.67917658501012917e+00;

static constexpr double a101 =  4.77662536438264366e-01;
static constexpr double a104 = -2.48811461997166764e+00;
static constexpr double a105 = -5.90290826836842996e-01;
static constexpr double a106 =  2.12300514481811942e+01;
static constexpr double a107 = -1.87717063725980367e+01;
static constexpr double a108 =  5.99004794812679254e+00;
static constexpr double a109 = -6.26835576062994894e-01;

static constexpr double a111 = -9.31463175788185445e-01;
static constexpr double a114 =  5.64011468450664607e+00;
static constexpr double a115 =  2.19149943912950647e+00;
static constexpr double a116 = -2.77767416803128227e+01;
static constexpr double a117 =  2.64920887616478832e+01;
static constexpr double a118 = -9.26750500674845044e+00;
static constexpr double a119 =  4.29400992168673737e-01;
static constexpr double a1110 = 1.26148050801826451e+00;

static constexpr double a121 =  2.27331014751653820e-01;
static constexpr double a124 = -1.05344954667372501e+01;
static constexpr double a125 = -2.00087205822486002e+00;
static constexpr double a126 = -1.79589318631187990e+01;
static constexpr double a127 =  2.79488845294199600e+01;
static constexpr double a128 = -2.85899827713502369e+00;
static constexpr double a129 = -8.87285693353062954e+00;
static constexpr double a1210 = 1.23605671757943034e+01;
static constexpr double a1211 = 6.43392880736874482e-01;

static constexpr double b1  =  5.42937341165687296e-02;
static constexpr double b6  =  4.45031289275240888e+00;
static constexpr double b7  =  1.89151789931450038e+00;
static constexpr double b8  = -5.8012039600105847e+00;
static constexpr double b9  =  3.1116436695781989e-01;
static constexpr double b10 = -1.52160949662516078e-01;
static constexpr double b11 =  2.01365400804030348e-01;
static constexpr double b12 =  4.47106157277725905e-02;

static constexpr double e1  =  0.1312004499419488073e-01;
static constexpr double e6  = -0.1225156446376204440e-05;
static constexpr double e7  = -0.4957589496572501915e-03;
static constexpr double e8  =  0.1664377182454986536e-02;
static constexpr double e9  = -0.3550085900823018509e-03;
static constexpr double e10 =  0.2341883419600798046e-03;
static constexpr double e11 =  0.3435651745687565260e-04;

} // namespace

some_ode_solver::some_ode_solver(std::size_t state_size, double initial_step)
    : some_ode_solver(state_size, initial_step, OdeMethod::RK4)
{
}

some_ode_solver::some_ode_solver(std::size_t state_size,
                                 double initial_step,
                                 OdeMethod method)
    : m_state_size(state_size),
      m_step(initial_step),
      m_last_step(initial_step),
      m_tol_rel(1e-10),
      m_tol_abs(1e-12),
      m_method(method)
{
    if (state_size == 0) {
        throw std::runtime_error("ODE solver: state size must be positive");
    }

    if (initial_step <= 0.0) {
        throw std::runtime_error("ODE solver: step must be positive");
    }
}

void some_ode_solver::set_method(OdeMethod method)
{
    m_method = method;
}

OdeMethod some_ode_solver::method() const
{
    return m_method;
}

void some_ode_solver::set_step_size(double step)
{
    if (step <= 0.0) {
        throw std::runtime_error("ODE solver: step must be positive");
    }

    m_step = step;
}

double some_ode_solver::step_size() const
{
    return m_step;
}

double some_ode_solver::last_step_size() const
{
    return m_last_step;
}

void some_ode_solver::set_tolerances(double tol_rel, double tol_abs)
{
    if (tol_rel <= 0.0 || tol_abs <= 0.0) {
        throw std::runtime_error("ODE solver: tolerances must be positive");
    }

    m_tol_rel = tol_rel;
    m_tol_abs = tol_abs;
}

void some_ode_solver::operator()(state_t& state, double t, deriv_t& deriv)
{
    double local_t = t;
    step(state, local_t, deriv);
}

void some_ode_solver::step(state_t& state, double& t, deriv_t& deriv)
{
    check_state_size(state);

    switch (m_method) {
    case OdeMethod::Euler:
        euler_step(state, m_step, deriv);
        m_last_step = m_step;
        t += m_last_step;
        break;

    case OdeMethod::Leapfrog:
        leapfrog_step(state, m_step, deriv);
        m_last_step = m_step;
        t += m_last_step;
        break;

    case OdeMethod::RK4:
        rk4_step(state, m_step, deriv);
        m_last_step = m_step;
        t += m_last_step;
        break;

    case OdeMethod::DP8:
        dp8_step(state, m_step, deriv);
        m_last_step = m_step;
        t += m_last_step;
        break;

    case OdeMethod::DP8Adaptive:
        m_last_step = dp8_adaptive_step(state, m_step, deriv);
        t += m_last_step;
        break;
    }
}

void some_ode_solver::check_state_size(const state_t& state) const
{
    if (state.size() != m_state_size) {
        throw std::runtime_error("ODE solver: wrong state size");
    }
}

void some_ode_solver::evaluate(deriv_t& deriv, const state_t& state, state_t& dst) const
{
    if (state.size() != m_state_size) {
        throw std::runtime_error("ODE solver: derivative input has wrong size");
    }

    dst.assign(m_state_size, 0.0);
    deriv(state, dst);

    if (dst.size() != m_state_size) {
        throw std::runtime_error("ODE solver: derivative output has wrong size");
    }
}

void some_ode_solver::euler_step(state_t& state, double h, deriv_t& deriv) const
{
    state_t k1;
    evaluate(deriv, state, k1);
    add_scaled(state, k1, h);
}

void some_ode_solver::rk4_step(state_t& state, double h, deriv_t& deriv) const
{
    state_t k1, k2, k3, k4;

    evaluate(deriv, state, k1);
    evaluate(deriv, make_stage_state(state, h, {{0.5, &k1}}), k2);
    evaluate(deriv, make_stage_state(state, h, {{0.5, &k2}}), k3);
    evaluate(deriv, make_stage_state(state, h, {{1.0, &k3}}), k4);

    for (std::size_t i = 0; i < m_state_size; ++i) {
        state[i] += h * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]) / 6.0;
    }
}

void some_ode_solver::leapfrog_step(state_t& state, double h, deriv_t& deriv) const
{
    if (m_state_size % 2 != 0) {
        throw std::runtime_error("Leapfrog requires state layout [positions, velocities]");
    }

    const std::size_t half = m_state_size / 2;

    state_t k;
    evaluate(deriv, state, k);

    // v_{n+1/2} = v_n + h/2 * a(r_n, v_n)
    for (std::size_t i = 0; i < half; ++i) {
        state[half + i] += 0.5 * h * k[half + i];
    }

    // r_{n+1} = r_n + h * v_{n+1/2}
    for (std::size_t i = 0; i < half; ++i) {
        state[i] += h * state[half + i];
    }

    // a_{n+1} = a(r_{n+1}, v_{n+1/2})
    evaluate(deriv, state, k);

    // v_{n+1} = v_{n+1/2} + h/2 * a_{n+1}
    for (std::size_t i = 0; i < half; ++i) {
        state[half + i] += 0.5 * h * k[half + i];
    }
}

void some_ode_solver::dp8_step(state_t& state, double h, deriv_t& deriv) const
{
    state_t k1, k2, k3, k4, k5, k6, k7, k8, k9, k10, k11, k12;

    evaluate(deriv, state, k1);
    evaluate(deriv, make_stage_state(state, h, {{a21, &k1}}), k2);
    evaluate(deriv, make_stage_state(state, h, {{a31, &k1}, {a32, &k2}}), k3);
    evaluate(deriv, make_stage_state(state, h, {{a41, &k1}, {a43, &k3}}), k4);
    evaluate(deriv, make_stage_state(state, h, {{a51, &k1}, {a53, &k3}, {a54, &k4}}), k5);
    evaluate(deriv, make_stage_state(state, h, {{a61, &k1}, {a64, &k4}, {a65, &k5}}), k6);
    evaluate(deriv, make_stage_state(state, h, {{a71, &k1}, {a74, &k4}, {a75, &k5}, {a76, &k6}}), k7);
    evaluate(deriv, make_stage_state(state, h, {{a81, &k1}, {a84, &k4}, {a85, &k5}, {a86, &k6}, {a87, &k7}}), k8);
    evaluate(deriv, make_stage_state(state, h, {{a91, &k1}, {a94, &k4}, {a95, &k5}, {a96, &k6}, {a97, &k7}, {a98, &k8}}), k9);
    evaluate(deriv, make_stage_state(state, h, {{a101, &k1}, {a104, &k4}, {a105, &k5}, {a106, &k6}, {a107, &k7}, {a108, &k8}, {a109, &k9}}), k10);
    evaluate(deriv, make_stage_state(state, h, {{a111, &k1}, {a114, &k4}, {a115, &k5}, {a116, &k6}, {a117, &k7}, {a118, &k8}, {a119, &k9}, {a1110, &k10}}), k11);
    evaluate(deriv, make_stage_state(state, h, {{a121, &k1}, {a124, &k4}, {a125, &k5}, {a126, &k6}, {a127, &k7}, {a128, &k8}, {a129, &k9}, {a1210, &k10}, {a1211, &k11}}), k12);

    for (std::size_t i = 0; i < m_state_size; ++i) {
        state[i] += h * (
            b1  * k1[i]  +
            b6  * k6[i]  +
            b7  * k7[i]  +
            b8  * k8[i]  +
            b9  * k9[i]  +
            b10 * k10[i] +
            b11 * k11[i] +
            b12 * k12[i]
        );
    }
}

double some_ode_solver::dp8_adaptive_step(state_t& state, double h, deriv_t& deriv)
{
    constexpr double h_min = 1e-8;
    constexpr double h_max = 1e-1;
    constexpr double safety = 0.9;
    constexpr double min_factor = 0.2;
    constexpr double max_factor = 5.0;

    h = std::clamp(h, h_min, h_max);

    while (true) {
        state_t original = state;

        state_t k1, k2, k3, k4, k5, k6, k7, k8, k9, k10, k11, k12;

        evaluate(deriv, original, k1);
        evaluate(deriv, make_stage_state(original, h, {{a21, &k1}}), k2);
        evaluate(deriv, make_stage_state(original, h, {{a31, &k1}, {a32, &k2}}), k3);
        evaluate(deriv, make_stage_state(original, h, {{a41, &k1}, {a43, &k3}}), k4);
        evaluate(deriv, make_stage_state(original, h, {{a51, &k1}, {a53, &k3}, {a54, &k4}}), k5);
        evaluate(deriv, make_stage_state(original, h, {{a61, &k1}, {a64, &k4}, {a65, &k5}}), k6);
        evaluate(deriv, make_stage_state(original, h, {{a71, &k1}, {a74, &k4}, {a75, &k5}, {a76, &k6}}), k7);
        evaluate(deriv, make_stage_state(original, h, {{a81, &k1}, {a84, &k4}, {a85, &k5}, {a86, &k6}, {a87, &k7}}), k8);
        evaluate(deriv, make_stage_state(original, h, {{a91, &k1}, {a94, &k4}, {a95, &k5}, {a96, &k6}, {a97, &k7}, {a98, &k8}}), k9);
        evaluate(deriv, make_stage_state(original, h, {{a101, &k1}, {a104, &k4}, {a105, &k5}, {a106, &k6}, {a107, &k7}, {a108, &k8}, {a109, &k9}}), k10);
        evaluate(deriv, make_stage_state(original, h, {{a111, &k1}, {a114, &k4}, {a115, &k5}, {a116, &k6}, {a117, &k7}, {a118, &k8}, {a119, &k9}, {a1110, &k10}}), k11);
        evaluate(deriv, make_stage_state(original, h, {{a121, &k1}, {a124, &k4}, {a125, &k5}, {a126, &k6}, {a127, &k7}, {a128, &k8}, {a129, &k9}, {a1210, &k10}, {a1211, &k11}}), k12);

        state_t next = original;
        for (std::size_t i = 0; i < m_state_size; ++i) {
            next[i] += h * (
                b1  * k1[i]  +
                b6  * k6[i]  +
                b7  * k7[i]  +
                b8  * k8[i]  +
                b9  * k9[i]  +
                b10 * k10[i] +
                b11 * k11[i] +
                b12 * k12[i]
            );
        }

        double err2 = 0.0;
        for (std::size_t i = 0; i < m_state_size; ++i) {
            const double err_deriv =
                e1  * k1[i]  +
                e6  * k6[i]  +
                e7  * k7[i]  +
                e8  * k8[i]  +
                e9  * k9[i]  +
                e10 * k10[i] +
                e11 * k11[i];

            const double scale =
                m_tol_abs + m_tol_rel * std::max(std::abs(original[i]), std::abs(next[i]));

            const double normalized = h * err_deriv / scale;
            err2 += normalized * normalized;
        }

        const double err = std::sqrt(err2 / static_cast<double>(m_state_size));

        double factor;
        if (err == 0.0) {
            factor = max_factor;
        } else {
            factor = safety * std::pow(1.0 / err, 1.0 / 8.0);
            factor = std::clamp(factor, min_factor, max_factor);
        }

        const double h_new = std::clamp(h * factor, h_min, h_max);

        if (err <= 1.0 || h <= h_min) {
            state = std::move(next);
            m_step = h_new;
            return h;
        }

        h = h_new;
    }
}
