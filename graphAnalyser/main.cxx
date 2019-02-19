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
#include "graph.h"
#include "metric.h"
#include "util.h"
#include "sizeMetric.cxx"
#include "sparsenessMetric.cxx"
#include "parse.h"
#include "metricset.h"

namespace po = boost::program_options;
namespace fs = std::filesystem;

#ifndef STORE_PATH
    #define STORE_PATH "./stoore"
#endif
#ifndef CONF_PATH
    #define CONF_PATH "./coonf"
#endif

std::set<std::string> graphFiles;
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
        ("force-store", po::bool_switch(), "Use the store's list of graphs despite the --graphs or --graph-dirs options being specified. (The union will be taken.)")
        ("force-recalculate,r", po::bool_switch(), "When an existing results file is found, do not trust the existing results and recalculate all metrics, overwriting any conflicting values.")
        ("clobber", po::bool_switch(), "When an existing results file is found for a given graph, truncate it before writing the new results.\nThis is useful if some metric has been deprecated and is not used anymore, but still clogs up the results files.");

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
        std::cout << "Command line options generally override config file options, except when they specify lists." << std::endl;
        std::cout << "List arguments get merged from both sources. Specify a non-existing config file to avoid this." << std::endl;
        std::cout << std::endl;
        std::cout << cmdOpts << std::endl;
        return 0;
    }

    /******** Validate store directory structure ********/
    //Maybe? Honestly probably not. At least not centrally: relevant parts will check it locally

    /******** Construct list of graphs ********/
    bool useStore = true;
    if (opts.count("graphs")) {
        useStore = false;
        std::vector<std::string> tmpGraphFiles = opts["graphs"].as<std::vector<std::string>>();
        for (std::string tmpgf : tmpGraphFiles) {
            graphFiles.insert(tmpgf);
        }
    }

    std::vector<std::string> graphDirs;
    if (opts.count("graph-dirs")) {
        useStore = false;
        graphDirs = opts["graph-dirs"].as<std::vector<std::string>>();
    }
    if (useStore || opts["forcestore"].as<bool>()) {
        graphDirs.push_back(opts["store-path"].as<std::string>() + "/graphs/");
    }

    for (std::string dir : graphDirs) {
        for (auto& dirent : fs::recursive_directory_iterator(dir, fs::directory_options::follow_directory_symlink)) {
            if (dirent.is_regular_file()) {
                graphFiles.insert(dirent.path().string());;
            }
        }
    }

    /******** Construct list of metrics ********/
    metrics.push_back(std::make_unique<SizeMetric>());
    metrics.push_back(std::make_unique<SparsenessMetric>());

    /******** Loop over graphs: parse graph, run metrics, save results ********/

    //Ensure output directory is good
    fs::path outDir(opts["store-path"].as<std::string>() + "/graph-scores/");
    if (fs::exists(outDir)) {
        if (!fs::is_directory(outDir)) {
            std::cerr << "Malformed store: entry \"/graph-scores/\" is not a directory" << std::endl;
            return 1;
        }
    } else {
        fs::create_directory(outDir);
    }

    //loop over graphs
    for (std::string graphFile : graphFiles) {
        const Graph& parsedGraph = *parseFile(graphFile);
        fs::path ofp = outDir / parsedGraph.hash();
        if (fs::exists(ofp) && !opts["force-recalculate"].as<bool>()) {
            std::cout << "INFO: graph " << graphFile << " has already been analysed. Skipping. "
                << "Use the --force-recalculate option to change this behaviour." << std::endl;
            continue;
        }

        //prepare results set
        MetricSet mset(ofp);

        if (opts["clobber"].as<bool>()) {
            mset.clear();
        }

        //check we can output
        std::ofstream graphOut(ofp.c_str());
        if (!graphOut) {
            std::cerr << "ERROR: Unable to write to graph output file " << ofp.string()
                << " for graph read from " << graphFile << ". Skipping graph!" << std::endl;
        }
        graphOut.close();

        //actually do the calculations for every metric
        for (const auto& m : metrics) {
            double score = m->calculate(parsedGraph);
            mset.setScore(m->name, score);
        }

        //finally save the output
        try {
            mset.save();
        } catch (std::exception& e) {
            std::cerr << "ERROR: Failure trying to write output file " << ofp.string() << " for graph " << graphFile << ". Exception message:" << std::endl;
            std::cerr << e.what() << std::endl;
            std::cerr << "Skipping graph!" << std::endl;
        }
    }
}
