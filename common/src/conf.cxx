#include "conf.h"
namespace po = boost::program_options;

void parseCmdLine(int ac, char** av) {
    desc.add_options()
        ("help", "produce help message");
    po::store(po::parse_command_line(ac, av, desc), opts);
    po::notify(opts);
}
