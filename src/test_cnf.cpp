
#include <iostream>
#include <algorithm>
#include <random>

#include <cnf/cnf.hpp>



const size_t MAX_TRY_COUNT = 10000;

int main() {

    CNF cnf("../bench/test/uf20-09.cnf");

    std::vector<bool> model(cnf.variable_count());

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);

    bool result;
    size_t counter = 1;
    while (counter <= MAX_TRY_COUNT && (result = cnf(model)) == false) {
        std::generate(model.begin(), model.end(), [&]() { return dis(gen); });
        counter++;
    }

    if (result) {
        std::cout << "SAT at attempt #" << counter << " model is: ";
        for (size_t i = 0; i < model.size() - 1; i++)
            std::cout << model[i] << ' ';
        std::cout << model.back() << std::endl;
    } else {
        std::cout << "FAIL: too many attempts" << std::endl;
    }
}

