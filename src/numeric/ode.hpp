#ifndef HPP_NUMERIC_ODE_
#define HPP_NUMERIC_ODE_

#include <cstddef>
#include <vector>
#include <functional>

using state_t = std::vector<double>;
using deriv_t = std::function<void(const state_t&, state_t&)>;

enum class OdeMethod {
    Leapfrog,
    RK4,
    RK8,
    DP8,              
    DP8Adaptive
};

class some_ode_solver {
public:
    some_ode_solver(std::size_t state_size,
                    double initial_step,
                    OdeMethod method = OdeMethod::RK4);
    
    void set_method(OdeMethod method);
    OdeMethod method() const;

    void set_step_size(double step);
    double step_size() const;
    double last_step_size() const;

    void set_tolerances(double tol_rel, double tol_abs);

    // Единственная чистая функция выполнения шага, обновляющая t и state
    void step(state_t& state, double& t, deriv_t& deriv);

private:
    void check_state_size(const state_t& state) const;
    void evaluate(deriv_t& deriv, const state_t& state, state_t& dst) const;

    void rk4_step(state_t& state, double h, deriv_t& deriv) const;
    void rk8_step(state_t& state, double h, deriv_t& deriv) const; // Схема Кутта 8-го порядка
    void leapfrog_step(state_t& state, double h, deriv_t& deriv) const;
    void dp8_step(state_t& state, double h, deriv_t& deriv) const;
    void dp8_adaptive_step(state_t& state, double& t, double& h, deriv_t& deriv);

    std::size_t m_state_size;
    double m_step_size;
    double m_last_step_size;
    OdeMethod m_method;

    double m_tol_rel;
    double m_tol_abs;
};

#endif // HPP_NUMERIC_ODE_