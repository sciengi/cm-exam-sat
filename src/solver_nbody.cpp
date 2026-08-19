#include <cnf/cnf.hpp>
#include <methods/nbody.hpp>
#include <numeric/ode.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>



int main(int argc, char** argv) {

	if (argc != 7) {
        std::cerr
            << "Usage:\n"
            << "\tnbody_solver <taskfile> <step_limit> <c_opp> <c_att> <c_cl> <gamma>"
			<< std::endl;
        return 1;
    }

    CNF cnf(argv[1]);

    const nbody::NBodyParams params = {
    		.dim      = 3, // DEV: constant for visualizer
			.c_opp    = std::stod(argv[3]),
			.c_att    = std::stod(argv[4]),
			.c_clause = std::stod(argv[5]),
			.gamma    = std::stod(argv[6])
    };

	auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();

	auto state = nbody::init_state(cnf.variable_count(), params, seed);
	auto deriv = nbody::build_deriv(cnf, params);

	const OdeMethod method = OdeMethod::RK4;
	const double step_size = 1e-3;
	const size_t step_limit = std::stoul(argv[2]);

	some_ode_solver solver(state.size(), step_size, method);

	const std::size_t decode_every = 100; // TODO: add to CLI
	double t = 0.0;

	// TODO: task setup event
	std::cout << "TASK(setup): "
			  << "file=" << std::quoted(argv[1])
			  << "dim" << params.dim
			  << "c_opp" << params.c_opp
			  << "c_att" << params.c_att
			  << "c_clause" << params.c_clause
			  << "gamma" << params.gamma
			  << "seed" << seed
			  << "step_size" << step_size
			  << "step_limit" << step_limit
			  << "decode_every" << decode_every
			  << std::endl;

	for (std::size_t step = 0; step < step_limit; ++step) {
		solver.step(state, t, deriv);

		for (const double& x : state) { // TODO: move to "system check" callback
			if (!std::isfinite(x)) {
				std::cout << "SYSTEM(status): crushed" << std::endl; // TODO: system crush event
				return 3;
			}
		}

		// TODO: system data event
		std::cout << "SYSTEM(data):";
		for (const double& v : state)
			std::cout << ' ' << v;
		std::cout << std::endl;

		if (step % decode_every == 0) {
			auto decoded = nbody::decode_best(cnf, state, params);

			if (decoded.decoded && decoded.sat) {
				std::cout << "TASK(status): SAT" << std::endl; // TODO: SAT event
				return 0;
			}
		}
	}

	std::cout << "CONSTRAIN(step_limit): reached, model not found" << std::endl; // TODO: step limit reached event

    return 2;
}
