#ifndef GRAPHHASHSET_H
#define GRAPHHASHSET_H

#include <unordered_map>
#include <fstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class GraphHashSet
{
    public:
        GraphHashSet();
        bool exists(std::string) const;
        std::string getHash(std::string) const;
        void setHash(std::string, std::string);
        void save() const;
    private:
        std::unordered_map<std::string, std::string> hashmap;
};

#endif
