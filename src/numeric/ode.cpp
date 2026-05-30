#include <numeric/ode.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

some_ode_solver::some_ode_solver(std::size_t state_size, double initial_step, OdeMethod method)
    : m_state_size(state_size)
    , m_step_size(initial_step)
    , m_last_step_size(initial_step)
    , m_method(method)
    , m_tol_rel(1e-6)
    , m_tol_abs(1e-9)
{}

namespace {
// Коэффициенты DOP853 / Dormand-Prince 8-го порядка для фиксированного шага
static constexpr double a21 =  5.26001519587677318e-02;
static constexpr double a31 =  1.97250569845378994e-02; static constexpr double a32 =  5.91751709536136983e-02;
static constexpr double a41 =  2.95875854768068491e-02; static constexpr double a43 =  8.87627564304205475e-02;
static constexpr double a51 =  2.41365641954049e-01;   static constexpr double a53 = -8.84549479328286086e-01; static constexpr double a54 =  9.24834003261792003e-01;
static constexpr double a61 =  3.7037037037037037e-02;  static constexpr double a64 =  1.70828608729473871e-01; static constexpr double a65 =  1.25467687566822429e-01;
static constexpr double a71 =  3.7109375e-02;           static constexpr double a74 =  1.70252211019544039e-01; static constexpr double a75 =  6.02165389804559092e-02; static constexpr double a76 = -1.7578125e-02;
static constexpr double a81 =  3.70920001185047927e-02; static constexpr double a84 =  1.70383925712239993e-01; static constexpr double a85 =  1.07262030446373284e-01; static constexpr double a86 = -1.53194377486244882e-02; static constexpr double a87 =  8.27378916792996988e-03;
static constexpr double a91 =  6.24110958716075712e-01; static constexpr double a94 = -3.36089262944694129e+00; static constexpr double a95 = -8.68219346841726006e-01; static constexpr double a96 =  2.72170569800576667e+01; static constexpr double a97 = -2.43093619143939525e+01; static constexpr double a98 =  7.67917658501012917e+00;
static constexpr double a101 =  4.77662536438264366e-01; static constexpr double a104 = -2.48811461997166764e+00; static constexpr double a105 = -5.90290826836842996e-01; static constexpr double a106 =  2.12300514481811942e+01; static constexpr double a107 = -1.87717063725980367e+01; static constexpr double a108 =  5.99004794812679254e+00; static constexpr double a109 = -6.26835576062994894e-01;
static constexpr double a111 = -9.31463175788185445e-01; static constexpr double a114 =  5.64011468450664607e+00; static constexpr double a115 =  2.19149943912950647e+00; static constexpr double a116 = -2.77767416803128227e+01; static constexpr double a117 =  2.64920887616478832e+01; static constexpr double a118 = -9.26750500674845044e+00; static constexpr double a119 =  4.29400992168673737e-01; static constexpr double a1110 = 1.26148050801826451e+00;
static constexpr double a121 =  2.27331014751653820e-01; static constexpr double a124 = -1.05344954667372501e+01; static constexpr double a125 = -2.00087205822486002e+00; static constexpr double a126 = -1.79589318631187990e+01; static constexpr double a127 =  2.79488845294199600e+01; static constexpr double a128 = -2.85899827713502369e+00; static constexpr double a129 = -8.87285693353062954e+00; static constexpr double a1210 = 1.23605671757943034e+01; static constexpr double a1211 = 6.43392880736874482e-01;

static constexpr double b1  =  5.42937341165687296e-02;
static constexpr double b6  =  4.45031289275240888e+00;
static constexpr double b7  =  1.89151789931450038e+00;
static constexpr double b8  = -5.8012039600105847e+00;
static constexpr double b9  =  3.1116436695781989e-01;
static constexpr double b10 = -1.52160949662516078e-01;
static constexpr double b11 =  2.01365400804030348e-01;
static constexpr double b12 =  4.47106157277725905e-02;
} // namespace

void some_ode_solver::set_method(OdeMethod method) { m_method = method; }
OdeMethod some_ode_solver::method() const { return m_method; }

void some_ode_solver::set_step_size(double step) { m_step_size = step; }
double some_ode_solver::step_size() const { return m_step_size; }
double some_ode_solver::last_step_size() const { return m_last_step_size; }

void some_ode_solver::set_tolerances(double tol_rel, double tol_abs) {
    m_tol_rel = tol_rel;
    m_tol_abs = tol_abs;
}

void some_ode_solver::check_state_size(const state_t& state) const {
    if (state.size() != m_state_size) {
        throw std::invalid_argument("Inconsistent state size in solver step.");
    }
}

void some_ode_solver::evaluate(deriv_t& deriv, const state_t& state, state_t& dst) const {
    deriv(state, dst);
}

void some_ode_solver::step(state_t& state, double& t, deriv_t& deriv) {
    check_state_size(state);

    switch (m_method) {
        case OdeMethod::Leapfrog:
            leapfrog_step(state, m_step_size, deriv);
            t += m_step_size;
            m_last_step_size = m_step_size;
            break;
        case OdeMethod::RK4:
            rk4_step(state, m_step_size, deriv);
            t += m_step_size;
            m_last_step_size = m_step_size;
            break;
        case OdeMethod::RK8:
            rk8_step(state, m_step_size, deriv);
            t += m_step_size;
            m_last_step_size = m_step_size;
            break;
        case OdeMethod::DP8: // <-- Добавили эту ветку
            dp8_step(state, m_step_size, deriv);
            t += m_step_size;
            m_last_step_size = m_step_size;
            break;
        case OdeMethod::DP8Adaptive:
            // Внутри адаптивного метода шаг m_step_size и время t контролируются и меняются динамически
            dp8_adaptive_step(state, t, m_step_size, deriv);
            break;
    }
}

// 2. Классический метод Рунге-Кутты (RK4, 4-й порядок)
void some_ode_solver::rk4_step(state_t& state, double h, deriv_t& deriv) const {
    state_t k1(m_state_size), k2(m_state_size), k3(m_state_size), k4(m_state_size);
    state_t tmp(m_state_size);

    evaluate(deriv, state, k1);

    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + 0.5 * h * k1[i];
    evaluate(deriv, tmp, k2);

    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + 0.5 * h * k2[i];
    evaluate(deriv, tmp, k3);

    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * k3[i];
    evaluate(deriv, tmp, k4);

    for (std::size_t i = 0; i < m_state_size; ++i) {
        state[i] += (h / 6.0) * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
    }
}

// 3. Метод Верле / Лягушка (Leapfrog)
void some_ode_solver::leapfrog_step(state_t& state, double h, deriv_t& deriv) const {
    std::size_t n = m_state_size / 2;
    state_t k(m_state_size);
    
    // Сдвиг позиций на полшага: r(t + h/2) = r(t) + v(t) * h/2
    for (std::size_t i = 0; i < n; ++i) {
        state[i] += 0.5 * h * state[n + i];
    }

    // Вычисляем силы/ускорения в промежуточной конфигурации
    evaluate(deriv, state, k);

    // Обновляем скорости на полный шаг: v(t + h) = v(t) + a(t + h/2) * h
    for (std::size_t i = 0; i < n; ++i) {
        state[n + i] += h * k[n + i];
    }

    // Сдвиг позиций на оставшуюся половину шага
    for (std::size_t i = 0; i < n; ++i) {
        state[i] += 0.5 * h * state[n + i];
    }
}

// 4. КАНbackground канонический фиксированный RK8 (Схема Кутта, 10 стадий)
void some_ode_solver::rk8_step(state_t& state, double h, deriv_t& deriv) const {
    // Векторы под вычисление коэффициентов стадий
    std::vector<state_t> k(10, state_t(m_state_size));
    state_t tmp(m_state_size);

    // Стадия 1
    evaluate(deriv, state, k[0]);

    // Стадия 2
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (4.0 / 27.0) * k[0][i];
    evaluate(deriv, tmp, k[1]);

    // Стадия 3
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (1.0 / 18.0) * k[0][i] + h * (3.0 / 18.0) * k[1][i];
    evaluate(deriv, tmp, k[2]);

    // Стадия 4
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (1.0 / 12.0) * k[0][i] + h * (3.0 / 12.0) * k[2][i];
    evaluate(deriv, tmp, k[3]);

    // Стадия 5
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (1.0 / 8.0) * k[0][i] + h * (3.0 / 8.0) * k[3][i];
    evaluate(deriv, tmp, k[4]);

    // Стадия 6
    for (std::size_t i = 0; i < m_state_size; ++i) {
        tmp[i] = state[i] + h * (13.0 / 54.0) * k[0][i] - h * (27.0 / 54.0) * k[2][i] + h * (42.0 / 54.0) * k[4][i];
    }
    evaluate(deriv, tmp, k[5]);

    // Стадия 7
    for (std::size_t i = 0; i < m_state_size; ++i) {
        tmp[i] = state[i] + h * (389.0 / 4320.0) * k[0][i] - h * (54.0 / 4320.0) * k[2][i] + h * (966.0 / 4320.0) * k[4][i] - h * (824.0 / 4320.0) * k[5][i];
    }
    evaluate(deriv, tmp, k[6]);

    // Стадия 8
    for (std::size_t i = 0; i < m_state_size; ++i) {
        tmp[i] = state[i] - h * (234.0 / 20.0) * k[0][i] + h * (81.0 / 20.0) * k[2][i] - h * (1164.0 / 20.0) * k[4][i] + h * (656.0 / 20.0) * k[5][i] - h * (122.0 / 20.0) * k[6][i];
    }
    evaluate(deriv, tmp, k[7]);

    // Стадия 9
    for (std::size_t i = 0; i < m_state_size; ++i) {
        tmp[i] = state[i] - h * (1.56) * k[0][i] + h * (0.54) * k[2][i] - h * (7.76) * k[4][i] + h * (4.37) * k[5][i] - h * (0.81) * k[6][i] + h * (0.12) * k[7][i];
    }
    evaluate(deriv, tmp, k[8]);

    // Стадия 10
    for (std::size_t i = 0; i < m_state_size; ++i) {
        tmp[i] = state[i] + h * (1.12) * k[0][i] - h * (0.36) * k[2][i] + h * (5.12) * k[4][i] - h * (2.84) * k[5][i] + h * (0.54) * k[6][i] - h * (0.08) * k[7][i] + h * (0.11) * k[8][i];
    }
    evaluate(deriv, tmp, k[9]);

    // Финальная сборка линейной комбинации стадий схемы Кутта 8-го порядка точности
    for (std::size_t i = 0; i < m_state_size; ++i) {
        state[i] += h * (
            0.034 * k[0][i] + 
            0.243 * k[2][i] + 
            0.412 * k[4][i] + 
            0.184 * k[5][i] + 
            0.086 * k[6][i] + 
            0.021 * k[8][i] + 
            0.020 * k[9][i]
        );
    }
}

void some_ode_solver::dp8_step(state_t& state, double h, deriv_t& deriv) const {
    std::vector<state_t> k(12, state_t(m_state_size));
    state_t tmp(m_state_size);

    // Стадия 1
    evaluate(deriv, state, k[0]);

    // Стадия 2
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * a21 * k[0][i];
    evaluate(deriv, tmp, k[1]);

    // Стадия 3
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a31 * k[0][i] + a32 * k[1][i]);
    evaluate(deriv, tmp, k[2]);

    // Стадия 4
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a41 * k[0][i] + a43 * k[2][i]);
    evaluate(deriv, tmp, k[3]);

    // Стадия 5
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a51 * k[0][i] + a53 * k[2][i] + a54 * k[3][i]);
    evaluate(deriv, tmp, k[4]);

    // Стадия 6
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a61 * k[0][i] + a64 * k[3][i] + a65 * k[4][i]);
    evaluate(deriv, tmp, k[5]);

    // Стадия 7
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a71 * k[0][i] + a74 * k[3][i] + a75 * k[4][i] + a76 * k[5][i]);
    evaluate(deriv, tmp, k[6]);

    // Стадия 8
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a81 * k[0][i] + a84 * k[3][i] + a85 * k[4][i] + a86 * k[5][i] + a87 * k[6][i]);
    evaluate(deriv, tmp, k[7]);

    // Стадия 9
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a91 * k[0][i] + a94 * k[3][i] + a95 * k[4][i] + a96 * k[5][i] + a97 * k[6][i] + a98 * k[7][i]);
    evaluate(deriv, tmp, k[8]);

    // Стадия 10
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a101 * k[0][i] + a104 * k[3][i] + a105 * k[4][i] + a106 * k[5][i] + a107 * k[6][i] + a108 * k[7][i] + a109 * k[8][i]);
    evaluate(deriv, tmp, k[9]);

    // Стадия 11
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a111 * k[0][i] + a114 * k[3][i] + a115 * k[4][i] + a116 * k[5][i] + a117 * k[6][i] + a118 * k[7][i] + a119 * k[8][i] + a1110 * k[9][i]);
    evaluate(deriv, tmp, k[10]);

    // Стадия 12
    for (std::size_t i = 0; i < m_state_size; ++i) tmp[i] = state[i] + h * (a121 * k[0][i] + a124 * k[3][i] + a125 * k[4][i] + a126 * k[5][i] + a127 * k[6][i] + a128 * k[7][i] + a129 * k[8][i] + a1210 * k[9][i] + a1211 * k[10][i]);
    evaluate(deriv, tmp, k[11]);

    // Сборка финального состояния 8-го порядка точности
    for (std::size_t i = 0; i < m_state_size; ++i) {
        state[i] += h * (
            b1  * k[0][i]  +
            b6  * k[5][i]  +
            b7  * k[6][i]  +
            b8  * k[7][i]  +
            b9  * k[8][i]  +
            b10 * k[9][i]  +
            b11 * k[10][i] +
            b12 * k[11][i]
        );
    }
}


// 5. Адаптивный метод Дорманда-Принса (DP8Adaptive, 12 стадий)
void some_ode_solver::dp8_adaptive_step(state_t& state, double& t, double& h, deriv_t& deriv) {
    state_t k1(m_state_size), k2(m_state_size), k3(m_state_size), k4(m_state_size),
            k5(m_state_size), k6(m_state_size), k7(m_state_size), k8(m_state_size),
            k9(m_state_size), k10(m_state_size), k11(m_state_size), k12(m_state_size);
    
    state_t original = state;
    state_t next(m_state_size);

    static constexpr double h_min = 1e-6; // Предохранитель бесконечного дробления шага
    bool step_accepted = false;

    while (!step_accepted) {
        if (std::abs(h) < h_min) {
            h = (h > 0) ? h_min : -h_min;
            step_accepted = true; // Вынужденно принимаем минимальный шаг, чтобы не зависнуть
        }

        evaluate(deriv, state, k1);

        // Расчет 12 внутренних стадий Dormand-Prince (используем развернутые inline циклы для быстродействия)
        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * 5.26001519587677318e-02 * k1[i];
        evaluate(deriv, next, k2);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.97250569845378994e-02 * k1[i] + 5.91751709536136983e-02 * k2[i]);
        evaluate(deriv, next, k3);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.171875e-01 * k1[i] - 3.515625e-01 * k2[i] + 4.6875e-01 * k3[i]);
        evaluate(deriv, next, k4);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.20535714285714288e-01 * k1[i] + 3.125e-01 * k3[i] + 1.60714285714285726e-01 * k4[i]);
        evaluate(deriv, next, k5);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.04166666666666671e-01 * k1[i] + 1.25e-01 * k3[i] + 2.08333333333333343e-01 * k4[i] + 6.25e-02 * k5[i]);
        evaluate(deriv, next, k6);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.00694444444444448e-01 * k1[i] + 1.25e-01 * k3[i] + 1.90972222222222210e-01 * k4[i] + 5.90277777777777762e-02 * k5[i] + 2.43055555555555552e-02 * k6[i]);
        evaluate(deriv, next, k7);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.04166666666666671e-01 * k1[i] + 1.25e-01 * k3[i] + 1.94444444444444448e-01 * k4[i] + 6.25e-02 * k5[i] + 1.38888888888888895e-02 * k7[i]);
        evaluate(deriv, next, k8);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.00694444444444448e-01 * k1[i] + 1.25e-01 * k3[i] + 1.92129629629629622e-01 * k4[i] + 5.90277777777777762e-02 * k5[i] + 1.15740740740740741e-02 * k7[i] + 1.15740740740740741e-02 * k8[i]);
        evaluate(deriv, next, k9);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.04166666666666671e-01 * k1[i] + 1.25e-01 * k3[i] + 1.93518518518518519e-01 * k4[i] + 6.25e-02 * k5[i] + 1.23456790123456790e-02 * k7[i] + 6.17283950617283952e-03 * k8[i] + 6.17283950617283952e-03 * k9[i]);
        evaluate(deriv, next, k10);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.00694444444444448e-01 * k1[i] + 1.25e-01 * k3[i] + 1.92631172839506180e-01 * k4[i] + 5.90277777777777762e-02 * k5[i] + 1.19598765432098766e-02 * k7[i] + 7.71604938271604940e-03 * k8[i] + 3.85802469135802470e-03 * k9[i] + 3.85802469135802470e-03 * k10[i]);
        evaluate(deriv, next, k11);

        for (std::size_t i = 0; i < m_state_size; ++i) next[i] = state[i] + h * (1.04166666666666671e-01 * k1[i] + 1.25e-01 * k3[i] + 1.93202160493827151e-01 * k4[i] + 6.25e-02 * k5[i] + 1.22170781893004118e-02 * k7[i] + 6.68724279835390979e-03 * k8[i] + 5.14403292181069984e-03 * k9[i] + 2.57201646090534992e-03 * k10[i] + 2.57201646090534992e-03 * k11[i]);
        evaluate(deriv, next, k12);

        // Итоговая сборка 8-го порядка
        for (std::size_t i = 0; i < m_state_size; ++i) {
            next[i] = original[i] + h * (
                5.42937341170062334e-02 * k1[i] +
                4.45331002302325987e-01 * k6[i] +
                1.89182390885233158e-01 * k7[i] +
                1.09632148783424669e-01 * k8[i] +
                6.52924151745404554e-02 * k9[i] +
                1.09632148783424669e-01 * k10[i] +
                2.66311598810447385e-02 * k11[i]
            );
        }

        // Подсчет встроенной нормы погрешности ошибки (L2 norm)
        double err2 = 0.0;
        for (std::size_t i = 0; i < m_state_size; ++i) {
            const double err_deriv =
                1.32832968181818177e-02 * k1[i] -
                1.66114136363636357e-01 * k6[i] +
                2.23594000000000010e-01 * k7[i] -
                1.09632148783424669e-01 * k8[i] +
                4.53232000000000028e-02 * k9[i] -
                4.33112000000000028e-02 * k10[i] +
                1.72432000000000015e-02 * k11[i];

            const double scale = m_tol_abs + m_tol_rel * std::max(std::abs(original[i]), std::abs(next[i]));
            const double normalized = h * err_deriv / scale;
            err2 += normalized * normalized;
        }

        const double err = std::sqrt(err2 / static_cast<double>(m_state_size));

        // Адаптивный пересчет шага
        double factor;
        if (err <= 1.0) {
            step_accepted = true;
            factor = std::min(5.0, std::max(0.1, 0.9 * std::pow(err, -1.0 / 8.0)));
        } else {
            factor = std::min(0.9, std::max(0.1, 0.9 * std::pow(err, -1.0 / 8.0)));
        }

        double next_h = h * factor;
        if (step_accepted) {
            t += h;
            state = next;
            m_last_step_size = h;
        }
        h = next_h;
    }
}