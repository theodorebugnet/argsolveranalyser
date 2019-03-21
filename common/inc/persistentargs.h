#ifndef PERSISTENARGS_H
#define PERSISTENARGS_H

#include <map>
#include <fstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class PersistentArgs
{
    public:
        PersistentArgs();
        bool exists(std::string, std::string) const;
        std::string getArg(std::string, std::string) const;
        void setArg(std::string, std::string, std::string);
        void save() const;
    private:
        std::map<std::string, std::map<std::string, std::string>> argCache;
};

#endif
