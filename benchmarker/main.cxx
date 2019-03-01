#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include <set>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include "util.h"
#include "graph.h"
#include "opts.h"

#ifndef CONF_PATH
    #define CONF_PATH "./mapper.coonf"
#endif

namespace po = boost::program_options;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    /******** Define configuration options ********/
    po::options_description cmdOnly("Command-line only options");
    addHelpAndConfOpts(cmdOnly, CONF_PATH);

    po::options_description allSrcs("All configuration");
    allSrcs.add_options()
        ("reference-solver,r", po::value<std::string>(), "A trusted correct solver, used to generate reference solutions for graphs when one isn't found.")
        ("solver-binary,s", po::value<std::string>(), "The path to the solver binary that will be invoked.")
        ("run-id,i", po::value<std::string>(), "A string identifying the current run, under which the results will be saved. Previous results under the same ID will be overwritten. The special value TIMESTAMP will use a unique timestamp to identify the run, useful for automated or routine runs. If this option is not present, the value \"default\" is used as the ID.")
        ("save-all,a", po::bool_switch(), "All solutions generated by the benchmarked solver will be saved to disk. (By default, only (fully or partially) incorrect ones are saved.")
        ("save-none,n", po::bool_switch(), "None of the solutions generated during the benchmark, not even incorrect ones, will be saved to disk. Overrides --save-all.")
        ("save-max-size,m", po::value<std::string>(), "Maximum size below which solutions will be saved to disk. Defaults to bytes; suffixes \"k\", \"M\" or \"G\" can be used for kibibytes, mebibytes and gibibytes respectively. Applies to all solutions. Unlimited by default.")
        ("save-correct-max-size,M", po::value<std::string>(), "Maximum size below which fully correct solutions will be saved to disk. Must be lower than --save-max-size. Only applies if the --save-all option is also set. As above, prefixes can be used to determine units. Unlimited by default.")
        ("problems,p", po::value<std::vector<std::string>>()->composing()->multitoken(), "A list of problems to be solved on all input graphs.");
    addGraphFileOpts(allSrcs);

    po::options_description cmdOpts;
    cmdOpts.add(cmdOnly).add(allSrcs);

    //parse
    try {
        po::store(po::command_line_parser(argc, argv).options(cmdOpts).run(), opts);
        po::notify(opts);

        if (!opts["noconf"].as<bool>()) {
            std::ifstream confFile(opts["conf"].as<std::string>());
            if (confFile) {
                po::store(po::parse_config_file(confFile, allSrcs), opts);
                po::notify(opts);
            }
        }

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Terminating." << std::endl;
        return 1;
    }

    /******** Handle parsed options ********/
    if (opts.count("help")) {
        std::cout << "Usage: benchmarker [options]" << std::endl;
        std::cout << "Command line options generally override config file options, except when they specify lists." << std::endl;
        std::cout << "List arguments get merged from both sources." << std::endl;
        std::cout << cmdOpts << std::endl;
        return 0;
    }
    
    /******** Loop over graphs ********/
    //For each graph, parse it to get the hash, then loop over problems
        //For each problem, check if there's a solution. If not, and a reference solver was specified, invoke it; otherwise print a warning and skip.
        //Now that a solution exists, run the benchmarked solver. Get results. Depending on options, save to disk.
        //Compare results here, generate correctness, save to disk.
}

