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
#include "graph.h"
#include "metric.h"
#include "options.h"
#include "sizeMetric.cxx"
#include "parse.h"

namespace po = boost::program_options;
namespace fs = std::filesystem;

#ifndef STORE_PATH
    #define STORE_PATH "./stoore"
#endif
#ifndef CONF_PATH
    #define CONF_PATH "./coonf"
#endif

std::vector<std::string> graphFiles;
std::vector<std::unique_ptr<Metric>> metrics;

int main(int argc, char** argv) {

    /******** Define configuration options ********/
    //only for command line
    po::options_description cmdOnly("Command-line only options");
    cmdOnly.add_options()
        ("help,h", "Show this help.")
        ("conf,c", po::value<std::string>()->default_value(CONF_PATH), "Path to configuration file")
        ("noconf", po::bool_switch(), "Do not parse any configuration file (to turn off the default one).");

    //for both cmdline and config file
    po::options_description allSrcs("All configuration");
    allSrcs.add_options()
        ("store-path,s", po::value<std::string>()->default_value(STORE_PATH), "Store directory")
        ("graphs,g", po::value<std::vector<std::string>>()->multitoken()->composing(), "A list of loose graph files. Disables the store.")
        ("graph-dirs,d", po::value<std::vector<std::string>>()->multitoken()->composing(), "A list of directories containing graph files. Disables the store.")
        ("forcestore,f", po::bool_switch(), "Use the store despite the --graphs or --graph-dirs options being specified.");

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
        std::cout << "Usage: graph_analyser [options]" << std::endl;
        std::cout << std::endl;
        std::cout << cmdOpts << std::endl;
        return 0;
    }

    std::cout << "STORE ROOT IS: " << opts["store-path"].as<std::string>()  << std::endl;

    /******** Validate store directory structure ********/


    /******** Construct list of graphs ********/
    bool useStore = true;
    if (opts.count("graphs")) {
        useStore = false;
        graphFiles = opts["graphs"].as<std::vector<std::string>>();
    }

    std::vector<std::string> graphDirs;
    if (opts.count("graph-dirs")) {
        useStore = false;
        graphDirs = opts["graph-dirs"].as<std::vector<std::string>>();
    }
    if (useStore || opts["forcestore"].as<bool>()) {
        graphDirs.push_back(opts["store-path"].as<std::string>());
    }

    for (std::string dir : graphDirs) {
        for (auto& dirent : fs::recursive_directory_iterator(dir, fs::directory_options::follow_directory_symlink)) {
            if (dirent.is_regular_file()) {
                graphFiles.push_back(dirent.path().string());;
            }
        }
    }

    /******** Construct list of metrics ********/
    metrics.push_back(std::make_unique<SizeMetric>());

    /******** Loop over graphs: parse graph, run metrics, save results ********/
    for (std::string graphFile : graphFiles) {
        const Graph parsedGraph = *parseFile(graphFile);
        for (const auto& m : metrics) {
            std::cout << "Graph " << graphFile << " with hash: " << parsedGraph.hash() << "; under metric " << m->name << " gives score " << m->calculate(parsedGraph) << std::endl;
        }
    }

    /* Metric* m = new SizeMetric(); */
    /* Argument a1 = Argument("a1", 0); */
    /* std::vector<Argument> args = std::vector<Argument>(); */
    /* std::map<Argument, Argument> attacks = std::map<Argument, Argument>(); */
    /* args.push_back(a1); */
    /* Graph g = Graph(args, attacks); */
    /* g.printArgs(); */
    /* std::cout << "Metric " << m->name << " gives score " << m->calculate(g) << std::endl; */
}
