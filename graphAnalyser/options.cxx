#include "options.h"
#include <iostream>
namespace po = boost::program_options;

/* po::options_description desc("Allowed options"); */
void parseCmdLine(int ac, char** av, po::options_description& desc) {
    desc.add_options()
        ("help", "produce help message");
    po::store(po::parse_command_line(ac, av, desc), opts);
    po::notify(opts);
}

/* void printDescToStdout() { */
/*     std::cout << desc << std::endl; */
/* } */
