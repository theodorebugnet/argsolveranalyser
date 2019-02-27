#include "opts.h"
po::variables_map opts;

#ifndef STORE_PATH
    #define STORE_PATH "./stoore"
#endif
#ifndef CONF_PATH
    #define CONF_PATH "./coonf"
#endif

void addGraphFileOpts(po::options_description& desc) {
    desc.add_options()
        ("store-path,s", po::value<std::string>()->default_value(STORE_PATH), "Store directory")
        ("graphs,g", po::value<std::vector<std::string>>()->multitoken()->composing(), "A list of loose graph files. Disables the store.")
        ("graph-dirs,d", po::value<std::vector<std::string>>()->multitoken()->composing(), "A list of directories containing graph files. Disables the store.")
        ("use-store", po::bool_switch(), "Use the store's list of graphs despite the --graphs or --graph-dirs options being specified. (The union will be taken.)\nThis does nothing if neither of those options are also specified, as then the store is used by default.");
}

void addHelpAndConfOpts(po::options_description& desc, std::string default_conf_path) {
    desc.add_options()
        ("help,h", "Show this help and exit.")
        ("conf,c", po::value<std::string>()->default_value(default_conf_path), "Path to configuration file")
        ("noconf", po::bool_switch(), "Do not parse any configuration file (to turn off the default one).");
}
