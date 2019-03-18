#ifndef OPTIONS_H
#define OPTIONS_H
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
namespace po = boost::program_options;

extern po::variables_map opts;

void addGraphFileOpts(po::options_description&);
void addQuietVerboseOpts(po::options_description& desc);
void addHelpAndConfOpts(po::options_description&, std::string);
#endif
