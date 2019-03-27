#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <exception>
#include <fstream>
#include <set>
#include <unordered_set>
#include "graph.h"
#include "opts.h"
#include "util.h"
#include "graphhashset.h"
#include "metricset.h"
#include "generator.h"
#include "evolutionGenerator.cxx"

#ifndef CONF_PATH
    #define CONF_PATH "./analyser.coonf"
#endif

namespace po = boost::program_options;
namespace fs = std::filesystem;

std::vector<std::unique_ptr<Generator>> generators;

int main(int argc, char** argv)
{
    /******** List of generators ********/
    generators.push_back(std::make_unique<EvolutionGenerator>());

    /******** Define configuration options ********/
    //only for command line
    po::options_description cmdOnly("Command-line only options");
    addHelpAndConfOpts(cmdOnly, CONF_PATH);

    //for both cmdline and config file
    po::options_description allSrcs("All configuration");
    allSrcs.add_options()
        ("list-types,l", po::bool_switch(), "Print the available report types and exit.\n")
        ("type,t", po::value<std::string>(), "Report type to generate.\n")
        ("solver,s", po::value<std::string>(), "The name of the solver to focus on.\n")
        ("start-date,r", po::value<std::string>(), "For report type Evolution: earliest date to consider.\n")
        ("end-date,R", po::value<std::string>(), "For report type Evolution: latest date to consider.\n");

    addQuietVerboseOpts(allSrcs);

    po::options_description cmdOpts;
    cmdOpts.add(cmdOnly).add(allSrcs);

    //TODO here: improve options: have `po::options_description Generator::get_options() const`, parse the -h/-l/-t options, and then based on -t dynamically load the correct options_description and parse it.

    //parse
    try
    {   po::store(po::command_line_parser(argc, argv).options(cmdOpts).run(), opts);
        po::notify(opts);

        if (!opts["noconf"].as<bool>())
        {   std::ifstream confFile(opts["conf"].as<std::string>());
            if (confFile)
            {   po::store(po::parse_config_file(confFile, allSrcs), opts);
                po::notify(opts);
            }
        }

    }
    catch (std::exception& e)
    {   std::cerr << e.what() << std::endl;
        std::cerr << "Terminating." << std::endl;
        return 1;
    }

    /******** Handle parsed options ********/
    if (opts.count("help"))
    {   std::cout << "Usage: report_generator [options]" << std::endl;
        std::cout << "Command line options override config file options." << std::endl;
        std::cout << cmdOpts << std::endl;
        return 0;
    }
    if (opts["list-types"].as<bool>())
    {   for (std::unique_ptr<Generator>& gen : generators)
        {   std::cout << gen->name << "\n"
                << "    " << gen->description << std::endl;
        }
        return 0;
    }
    
    if (!opts.count("type"))
    {   std::cerr << "ERROR: No report type specified. Exiting." << std::endl;
        return 1;
    }

    bool valid_type = false;
    for (std::unique_ptr<Generator>& gen : generators)
    {   if (gen->name == opts["type"].as<std::string>())
        {   gen->run();
            valid_type = true;
            break;
        }
    }

    if (!valid_type)
    {   std::cerr << "ERROR: Invalid report type specified. Please use the --list-types option to view available types." << std::endl;
        return 1;
    }

    return 0;
}
