#ifndef HPP_UTILS_CLI_PARSER_
#define HPP_UTILS_CLI_PARSER_

#include <string>
#include <memory>

#include <methods/methods.hpp>

/* TODO:
 * - Add flag for ODE solver ant its params
 * - Make specification about help msg (add --help flags)
 */


namespace Cli {

    struct GeneralConfig {
        std::string target;
        size_t log_every;
        size_t decode_every;
        size_t max_step;
        double initial_step;
    };

    std::ostream& operator<<(std::ostream& stream, const Cli::GeneralConfig& conf);

    std::pair<GeneralConfig, std::unique_ptr<Method>> 
    Parse(int argc, char** argv);

}

#endif
