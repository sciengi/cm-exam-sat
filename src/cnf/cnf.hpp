#ifndef HPP_CNF_
#define HPP_CNF_

#include <filesystem>
#include <vector>
#include <memory>
#include <span>



class CNF {
    public:
        using model = std::vector<bool>;  // DEV: make std::span?
        static const size_t var_in_clause = 3;

    public:
        CNF(std::filesystem::path dimacscnf_file);

        size_t variable_count() const;

        size_t clause_count() const;

        std::span<const int, CNF::var_in_clause> operator[](int i);

        bool operator()(const model& model);

    private:
        size_t m_variable_count;
        size_t m_clause_count;
        size_t m_data_size;
        std::unique_ptr<int[]> m_data;
};

#endif
