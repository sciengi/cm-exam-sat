#include <cnf/cnf.hpp>



CNF::CNF(std::filesystem::path dimacscnf_file) { }

size_t CNF::variable_count() const { return 10; }

bool CNF::operator()(const std::vector<bool>& model) { return std::count(model.begin(), model.end(), true) > 5; }
