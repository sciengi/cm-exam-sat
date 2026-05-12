# Как подключить численные методы

Файлы:

- `ode.hpp` положить в `src/numeric/ode.hpp`
- `some_ode_solver.cpp` положить в `src/numeric/some_ode_solver.cpp`

В `CMakeLists.txt` для нужного executable должен быть файл:

```cmake
src/numeric/some_ode_solver.cpp
```

Пример использования:

```cpp
some_ode_solver solver(state.size(), 1e-3, OdeMethod::RK4);

double t = 0.0;
for (std::size_t step = 0; step < max_steps; ++step) {
    solver.step(state, t, deriv); // t увеличится сам
}
```

Для N-body Leapfrog требует layout:

```text
state = [positions..., velocities...]
```

То есть первая половина вектора — координаты, вторая половина — скорости.

Для DP8 Adaptive:

```cpp
some_ode_solver solver(state.size(), 1e-3, OdeMethod::DP8Adaptive);
solver.set_tolerances(1e-10, 1e-12);

double t = 0.0;
solver.step(state, t, deriv); // будет принят адаптивный шаг
```

Старый интерфейс тоже работает:

```cpp
solver(state, t, deriv);
t += solver.last_step_size();
```
