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
        ("hash-lookup,l", po::value<std::vector<std::string>>()->multitoken()->composing(), "List of file hashes to look up the source file of, out of the input files. Disables listing the hashes of the input files.")
        ("force-list-hashes,f", po::bool_switch(), "Forces listing the hashes of the input files, even if --hash-lookup is also specified.")
        ("pretty-print,p", po::bool_switch(), "Pads out the output into nice columns");
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
        std::cout << "Usage: graph_mapper [options]" << std::endl;
        std::cout << "Command line options generally override config file options, except when they specify lists." << std::endl;
        std::cout << "List arguments get merged from both sources." << std::endl;
        std::cout << cmdOpts << std::endl;
        return 0;
    }

    std::vector<std::string> graphFiles = get_graphlist();
    std::unordered_multimap<std::string, std::string> graphHashes;

    bool searchHashes = false;
    bool listHashes = true;
    if (!opts["hash-lookup"].empty()) {
        searchHashes = true;
        listHashes = false;
        if (opts["force-list-hashes"].as<bool>()) {
            listHashes = true;
        }
    }

    unsigned long maxfnamelen = 0;
    if (!graphFiles.empty()) {
        for (std::string graphFile : graphFiles) {
            if (graphFile.length() > maxfnamelen) {
                maxfnamelen = graphFile.length();
            }
            Graph* graphPtr = parseFile(graphFile);
            if (!graphPtr) {
                std::cerr << "ERROR: Error parsing graph file: " << graphFile <<". Skipping." << std::endl;
                continue;
            }
            graphHashes.insert(std::make_pair<std::string, std::string>(graphPtr->hash(), std::move(graphFile)));
            delete graphPtr;
        }
    }

    if (listHashes) {
        for (auto& entry : graphHashes) {
            std::cout << std::left << std::setw(maxfnamelen + 1) << entry.second << " " << entry.first << std::endl; //format: "graphname hashvalue" (space-sparated, one per line)
        }
    }
    if (searchHashes) {
        if (listHashes) { //we need a separator
            std::cout << "### Lookup results" << std::endl;
        }
        std::vector<std::string> searches = opts["hash-lookup"].as<std::vector<std::string>>();
        for (std::string search : searches) {
            auto results = graphHashes.equal_range(search);
            std::for_each(results.first, results.second, [](auto& res) {
                    std::cout << res.first << " " << res.second << std::endl;
                });
        }
    }
}

