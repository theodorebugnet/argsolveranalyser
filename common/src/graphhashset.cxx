#include <exception>
#include <stdexcept>
#include <iostream>
#include "graphhashset.h"
#include "opts.h"

GraphHashSet::GraphHashSet()
{
    fs::path hashmapfile (opts["store-path"].as<std::string>() + "/graphhashmap");
    if (!fs::exists(hashmapfile)) //no scores yet
    {   return;
    }

    std::ifstream infile(hashmapfile.c_str());
    if (!infile)
    {   std::cerr << "WARNING: unable to open existing hash cache file " << hashmapfile.string() << ". Hashes will not be cached, and if the --use-hash-cache option was set, cache will still not be used." << std::endl;
    }

    for (std::string namebuff, hashbuff; std::getline(infile, namebuff, ' ') && std::getline(infile, hashbuff);)
    {   hashmap[namebuff] = hashbuff;
    }
    infile.close();
}

bool GraphHashSet::exists(std::string graphFile) const
{
    return hashmap.find(graphFile) != hashmap.end();
}

std::string GraphHashSet::getHash(std::string graphFile) const
{
    return hashmap.at(graphFile);
}

void GraphHashSet::setHash(std::string graphFile, std::string hash)
{
    hashmap[graphFile] = hash;
}

void GraphHashSet::save() const
{
    std::string hashmappath = opts["store-path"].as<std::string>() + "/graphhashmap";
    std::ofstream ofile(hashmappath.c_str());
    if (!ofile)
    {   std::cerr << "WARNING: unable to open hash cache file for writing: " << hashmappath << ". Hashes generated or modified during this run will not be cached." << std::endl;
    }

    for (auto& entry : hashmap)
    {   ofile << entry.first << ' ' << entry.second << std::endl;
    }
    ofile.close();
}
