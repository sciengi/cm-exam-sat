#include <iomanip>
#include <utils/cli_parsers.hpp>

#include <stdexcept>
#include <format>
#include <string_view>
#include <getopt.h>


std::ostream& Cli::operator<<(std::ostream& stream, const Cli::GeneralConfig& conf) {
    return stream
        << "initial_step=" << conf.initial_step        << ' '
        << "max_step="     << conf.max_step            << ' '
        << "log_every="    << conf.log_every           << ' '
        << "decode_every=" << conf.decode_every        << ' '
        << "target="       << std::quoted(conf.target); 
}


static std::unique_ptr<Method> _ParseSolutionMethod(int argc, char** argv) {

    if (argc < 4) throw std::runtime_error("solution <alpha> <beta> <lambda> <mu>");

    auto m = std::make_unique<SolutionMethod>();
    
    m->alpha  = std::stod(argv[0]);
    m->beta   = std::stod(argv[1]);
    m->lambda = std::stod(argv[2]);
    m->mu     = std::stod(argv[3]);

    return m;
}
    

static std::unique_ptr<Method> _ParseNbodyMethod(int argc, char** argv) { 
    throw std::runtime_error("not implemented yet"); 
}


std::pair<Cli::GeneralConfig, std::unique_ptr<Method>> 
Cli::Parse(int argc, char** argv) {

    // Tip: default value set here

    GeneralConfig conf = {
        .target       = "",     // -t
        .log_every    = 1,      // -f
        .decode_every = 1,      // -d 
        .max_step     = 1'000,  // -M
        .initial_step = 1e-3    // -i
    };

    std::unique_ptr<Method> m;  // -m

    std::string fmt;
    long int    li_var;
    double      d_var;
    char        c_var;
    
    bool method_parsed = false;
    
    int opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, ":t:f:d:M:i:m:")) != -1 && !method_parsed) {
        switch (opt) {
            case 't':
                conf.target = optarg;
                break;

            case 'f':
                li_var = std::stol(optarg);
                if (li_var < 0) throw std::invalid_argument("log frequency cant be negative");
                conf.log_every = static_cast<size_t>(li_var);
                break;

            case 'd':
                li_var = std::stol(optarg);
                if (li_var < 0) throw std::invalid_argument("decode frequency cant be negative");
                conf.decode_every = static_cast<size_t>(li_var);
                break;


            case 'M':
                li_var = std::stol(optarg);
                if (li_var <= 0) throw std::invalid_argument("max step count cant be zero or less");
                conf.max_step = static_cast<size_t>(li_var);
                break;

            case 'i':
                d_var = std::stod(optarg);
                if (d_var <= 0) throw std::invalid_argument("initial step cant be zero or less");
                conf.initial_step = d_var;
                break;

            case 'm':
                if      (std::string_view("solution") == optarg) 
                    m = _ParseSolutionMethod(argc - optind, argv + optind);  // Notice: argv for _Parse* dont have program name

                else if (std::string_view("nbody")    == optarg) 
                    m = _ParseNbodyMethod(argc - optind, argv + optind);

                else {
                    fmt = "unrecognized method `{}`";
                    throw std::invalid_argument(std::vformat(fmt, std::make_format_args(optarg)));
                }

                method_parsed = true; // TODO: remove early loop end, read after _Parse*Method
                break;

            case '?':
                fmt = "invalid option `{}`";
                c_var = static_cast<char>(optopt);
                throw std::runtime_error(std::vformat(fmt, std::make_format_args(c_var)));

            case ':':
                fmt = "missing value for `{}`";
                throw std::runtime_error(std::vformat(fmt, std::make_format_args(argv[optind - 1])));
        };
    }

    if (conf.target.empty()) { throw std::runtime_error("require target"); }
    if (m == nullptr)        { throw std::runtime_error("require method"); }

    return { std::move(conf), std::move(m) };
}
