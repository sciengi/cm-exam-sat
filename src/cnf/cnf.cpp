#include <cnf/cnf.hpp>

#include <algorithm>


CNF::CNF([[maybe_unused]] std::filesystem::path dimacscnf_file) { }

size_t CNF::variable_count() const { return 10; }

bool CNF::operator()(const CNF::model& model) { return std::count(model.begin(), model.end(), true) > 5; }
