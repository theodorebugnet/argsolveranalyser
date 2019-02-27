#include <string>
#include <memory>
#include <vector>
#include "graph.h"

std::vector<std::string> get_graphlist();
std::set<std::string> get_graphset();
Graph* parseFile(std::string path);
