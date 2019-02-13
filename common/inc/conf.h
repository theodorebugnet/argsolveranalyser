#ifndef CONF_H
#define CONF_H

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/parsers.hpp>
namespace po = boost::program_options;

po::variables_map opts;
po::options_description desc("Allowed options");
void parseCmdLine(int, char**);

#endif
