#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
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

constexpr int MAX_VALIDATE_SIZE=1024*1024/2 //half a MB

std::vector<std::string> graphFiles;
std::vector<Metric> metrics;

/* Some very basic validation */
bool is_valid_tgf(fs::path filepath) {
    if (fs::file_size(filepath) < MAX_VALIDATE_SIZE) {
        std::ifstream file(filepath);
        std::string tokenbuff;
        enum States {arguments, attacks};
        States state = arguments;
        while (file >> tokenbuff) {
            if 
        }
    }
}

int main(int argc, char** argv) {

    /* Define configuration options */
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message");

    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);

    /* Handle parsed options */
    if (opts.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    /* Construct list of graphs */
    for (auto fp: fs::recursive_directory_iterator(STORE_PATH, fs::directory_options::follow_directory_symlink)) {
        if (fs::is_regular_file(fp)) {
            std::cout << fp << std::endl;
        }
    }

    /* Construct list of metrics */
    //TODO

    /* Loop over graphs: parse graph, run metrics, save results */

    /* Metric* m = new SizeMetric(); */
    /* Argument a1 = Argument("a1", 0); */
    /* std::vector<Argument> args = std::vector<Argument>(); */
    /* std::map<Argument, Argument> attacks = std::map<Argument, Argument>(); */
    /* args.push_back(a1); */
    /* Graph g = Graph(args, attacks); */
    /* g.printArgs(); */
    /* std::cout << "Metric " << m->name << " gives score " << m->calculate(g) << std::endl; */
}
