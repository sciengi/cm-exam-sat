#ifndef HPP_NUMERIC_ODE_
#define HPP_NUMERIC_ODE_

#include <cstddef>

#include <methods/solution.hpp>  // state_t = std::vector<double>, deriv_t = std::function<void(...)>

enum class OdeMethod {
    Euler,
    Leapfrog,
    RK4,
    DP8,
    DP8Adaptive
};

class some_ode_solver {
public:
    // Старый интерфейс сохранён: some_ode_solver(state.size(), h)
    // По умолчанию берём RK4, потому что он подходит и для общего ОДУ первого порядка,
    // и для N-body состояния [positions, velocities].
    some_ode_solver(std::size_t state_size,
                    double initial_step,
                    OdeMethod method = OdeMethod::RK4);
    
    some_ode_solver(std::size_t state_size, double initial_step);
    
    void set_method(OdeMethod method);
    OdeMethod method() const;

    void set_step_size(double step);
    double step_size() const;
    double last_step_size() const;

    void set_tolerances(double tol_rel, double tol_abs);


    void operator()(state_t& state, double t, deriv_t& deriv);

    void step(state_t& state, double& t, deriv_t& deriv);

private:
    void check_state_size(const state_t& state) const;
    void evaluate(deriv_t& deriv, const state_t& state, state_t& dst) const;

    void euler_step(state_t& state, double h, deriv_t& deriv) const;
    void rk4_step(state_t& state, double h, deriv_t& deriv) const;
    void leapfrog_step(state_t& state, double h, deriv_t& deriv) const;
    void dp8_step(state_t& state, double h, deriv_t& deriv) const;

    // Возвращает реально принятый шаг, а m_step обновляет на рекомендуемый следующий шаг.
    double dp8_adaptive_step(state_t& state, double h, deriv_t& deriv);

private:
    std::size_t m_state_size;
    double m_step;
    double m_last_step;
    double m_tol_rel;
    double m_tol_abs;
    OdeMethod m_method;
};

#endif
