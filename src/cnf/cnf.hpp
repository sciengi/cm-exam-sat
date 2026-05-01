#ifndef HPP_CNF_
#define HPP_CNF_

#include <filesystem>
#include <vector>



class CNF {
    public:
        using model = std::vector<bool>;

        CNF(std::filesystem::path dimacscnf_file);

        size_t variable_count() const;

        size_t clause_count() const;

        bool operator()(const model& model);

        class clause {
            int operator[](int i);
        };

        clause& operator[](int i);

};

#endif
