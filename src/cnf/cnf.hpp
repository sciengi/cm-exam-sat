#ifndef HPP_CNF_
#define HPP_CNF_

#include <filesystem>
#include <vector>
#include <algorithm>



class CNF {
    public:
        CNF(std::filesystem::path dimacscnf_file);

        size_t variable_count() const;

        bool operator()(const std::vector<bool>& model);
};

#endif
