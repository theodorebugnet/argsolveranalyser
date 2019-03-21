#include <exception>
#include <stdexcept>
#include <iostream>
#include "persistentargs.h"
#include "opts.h"

PersistentArgs::PersistentArgs()
{
    fs::path argCacheFile (opts["store-path"].as<std::string>() + "/randomargcache");
    if (!fs::exists(argCacheFile)) //no scores yet
    {   return;
    }

    std::ifstream infile(argCacheFile.c_str());
    if (!infile)
    {   std::cerr << "WARNING: Unable to open cache of previously used random arguments. New random arguments will be selected for decision problems. Abort now if you wish to reproduce previous results." << std::endl;
    }

    for (std::string hashbuff, problembuff, argbuff; std::getline(infile, hashbuff, ' ') && std::getline(infile, problembuff, ' ') && std::getline(infile, argbuff);)
    {   argCache[hashbuff][problembuff] = argbuff;
    }
    infile.close();
}

bool PersistentArgs::exists(std::string hash, std::string problem) const
{
    auto val = argCache.find(hash);
    if (val != argCache.end())
    {   return val->second.find(problem) != val->second.end();
    }
    return false;
}

std::string PersistentArgs::getArg(std::string hash, std::string problem) const
{
    return argCache.at(hash).at(problem);
}

void PersistentArgs::setArg(std::string hash, std::string problem, std::string arg)
{
    argCache[hash][problem] = arg;
}

void PersistentArgs::save() const
{
    fs::path argCacheFile (opts["store-path"].as<std::string>() + "/randomargcache");
    std::ofstream ofile(argCacheFile.c_str());
    if (!ofile)
    {   std::cerr << "WARNING: Unable to open random argument cache file for writing. Any arguments randomly selected for decision problems will not be saved (and therefore any results obtained may not be deterministically reproducible)." << std::endl;
    }

    for (auto& entry : argCache)
    {   for (auto& subentry : entry.second)
        {   ofile << entry.first << ' ' << subentry.first << ' ' << subentry.second << std::endl;
        }
    }
    ofile.close();
}
