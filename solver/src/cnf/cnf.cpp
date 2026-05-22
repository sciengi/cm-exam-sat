#include <cnf/cnf.hpp>

#include <fstream>



CNF::CNF(std::filesystem::path dimacscnf_file) {

    std::ifstream file(dimacscnf_file);
    if (!file.is_open()) {
        throw std::runtime_error("Cant open a file");
    }

    // DEV: suppose DIMACS format as 
    // [c comment body]* 
    // p cnf [var_count] [cl_count]
    // [clauses]+
    //
    // - No comments between clauses and after task

    char cmd;
    while (file >> cmd && cmd == 'c') {
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    // DEV: stream can crush, so check file.fail(), ...


    if (file.eof() || cmd != 'p') { 
        throw std::runtime_error("No task found");
    }
    
    // assert(cmd == 'p'); 

    file.ignore(5); // _cnf_
    file >> m_variable_count >> m_clause_count;
    file.ignore();


    m_data_size = var_in_clause * m_clause_count;
    m_data = std::make_unique<int[]>(m_data_size);
    
    int literal_index;
    size_t i = 0;

    while (file >> literal_index && i < m_data_size) {
        if (literal_index == 0) {
            continue; // TODO: add mixed task checking 
        }

        m_data[i++] = literal_index;
    }

    if (i != m_data_size) {
        throw std::runtime_error("Parse error");
    }

    // DEV: can check rest of a file
   
    /*
    std::cout << m_variable_count << m_clause_count << std::endl; 
    for (size_t j = 0; j < m_data_size; j++)
        std::cout << m_data[j] << std::endl;
    */
}


size_t CNF::variable_count() const { return m_variable_count; }


size_t CNF::clause_count() const { return m_clause_count; }


std::span<const int, CNF::var_in_clause> CNF::operator[](int i) const {
    return std::span<const int, var_in_clause>{m_data.get() + i * var_in_clause, var_in_clause}; 
}


bool CNF::operator()(const CNF::model& model) const {
  
    bool cl_result;
    for (size_t i = 0; i < clause_count(); i++) {
        const auto& cl = (*this)[i];
        cl_result = false;
        for (size_t q = 0; q < var_in_clause; q++) {
            const auto& index = cl[q];
            cl_result = cl_result || (index > 0 ? model[index - 1] : !model[-index - 1]);
        }

        if (!cl_result) { return false; }
    }

    return true;
}
