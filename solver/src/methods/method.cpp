#include <methods/method.hpp>



bool Method::PostProcessState(state_t& state) { return false; }



std::ostream& operator<<(std::ostream& stream, const Method& m) {
    m.Print(stream); return stream;
} 

