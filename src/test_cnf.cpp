
#include <iostream>
#include <algorithm>
#include <random>

#include <cnf/cnf.hpp>



const size_t MAX_TRY_COUNT = 100;

int main() {

    CNF cnf("DUMMY_PATH_TO_CNF");

    std::vector<bool> model(cnf.variable_count());

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);

    size_t counter = 1;
    while (counter <= MAX_TRY_COUNT && !cnf(model)) {
        std::generate(model.begin(), model.end(), [&]() { return dis(gen); });
        counter++;
    }

    if (counter > MAX_TRY_COUNT)
        std::cout << "Too many attempts" << std::endl;
    else
        std::cout << "Found solution at attempt #" << counter << std::endl;
}

