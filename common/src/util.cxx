#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include <set>
#include <algorithm>
#include <filesystem>
#include <random>
#include "graph.h"
#include "util.h"
#include "SpookyV2.h"
#include "opts.h"

namespace fs = std::filesystem;

constexpr uint64_t seed1 = 1;
constexpr uint64_t seed2 = 2;

std::vector<std::string> get_graphlist()
{
    std::vector<std::string> ret;
    bool useStore = true;
    std::vector<std::string> tmpGraphFiles;
    if (!opts["graphs"].empty())
    {   useStore = false;
        tmpGraphFiles = opts["graphs"].as<std::vector<std::string>>();
        for (std::string tmpgf : tmpGraphFiles)
        {   ret.push_back(tmpgf);
        }
    }
    std::vector<std::string> graphDirs;
    if (!opts["graph-dirs"].empty())
    {   useStore = false;
        graphDirs = opts["graph-dirs"].as<std::vector<std::string>>();
    }
    if (useStore || opts["use-store"].as<bool>())
    {   if (!opts["store-path"].empty())
        {   graphDirs.push_back(opts["store-path"].as<std::string>() + "/graphs/");
        }
    }
    for (std::string dir : graphDirs)
    {   for (auto& dirent : fs::recursive_directory_iterator(dir, fs::directory_options::follow_directory_symlink))
        {   if (dirent.is_regular_file())
            {   ret.push_back(dirent.path().string());;
            }
        }
    }

    return ret;
}

std::set<std::string> get_graphset()
{
    std::set<std::string> ret;
    std::vector<std::string> tmp = get_graphlist();
    for (auto& g : tmp)
    {   ret.insert(g);
    }
    return ret;
}

Graph* parseFile(std::string path)
{
    std::map<std::string, std::shared_ptr<Argument>> arglookup;
    std::set<std::pair<std::string, std::string>> atts;
    std::ifstream ifile(path);
    std::string tokenbuff, secondbuff;
    enum States : bool { arguments, attacks };
    States state = arguments;

    for (int ctr = 0; ifile >> tokenbuff; ctr++)
    {   switch (state)
        {   case arguments:
                if (tokenbuff == "#") state = attacks;
                else
                {   auto insert = arglookup.insert(std::make_pair<std::string, std::shared_ptr<Argument>>(std::move(tokenbuff), std::make_shared<Argument>(tokenbuff, ctr)));
                    if (!insert.second)
                    {   std::cerr << "WARNING: Malformed tgf file: duplicate argument on line " << ctr << ". Ignoring." << std::endl;
                    }
                }
                break;
            case attacks:
                if (!(ifile >> secondbuff))
                {   std::cerr << "ERROR: Malformed tgf file: an attack is missing its destination" << std::endl;
                    return nullptr;
                }
                std::pair<std::set<std::pair<std::string, std::string>>::iterator, bool> insert = atts.insert(std::make_pair(tokenbuff, secondbuff));
                if (!insert.second)
                {   std::cerr << "WARNING: Malformed tgf file: duplicate attack on line " << ctr << ". Ignoring." << std::endl;
                }
                break;
        }
    }

    Graph* ret = new Graph(path);
    SpookyHash graphHash;
    graphHash.Init(seed1, seed2);

    for (auto& arg : arglookup)
    {   graphHash.Update(arg.first.c_str(), arg.first.length());
        ret->addArgument(arg.second);
    }
    for (std::pair<std::string, std::string> att : atts)
    {   graphHash.Update((att.first + att.second).c_str(), (att.first + att.second).length());
        ret->addAttack(arglookup[att.first], arglookup[att.second]);
    }

    uint64_t hash1;
    uint64_t hash2;
    graphHash.Final(&hash1, &hash2);
    std::stringstream streamtmp;
    streamtmp << std::hex << hash1 << hash2;
    ret->setHash(streamtmp.str());
    return ret;
}

std::string getAdditionalArg(std::string graphFile, std::string arg)
{
    std::string additionalArr;
    std::ifstream gin(graphFile);
    std::string argbuff, prevbuff;

    if (arg != "" && arg != "-")
    {   unsigned long argpos = 0;
        try
        {   argpos = std::stol(arg);
            for (unsigned long ctr = 0; std::getline(gin, argbuff); ctr++)
            {   if (argbuff == "#")
                {   additionalArr = prevbuff;
                    break;
                }
                else if (ctr == argpos)
                {   additionalArr = argbuff;
                    break;
                }
            }
            return additionalArr;
        }
        catch (std::exception& e)
        {   return arg;
        }
    }

    //else
    else
    {   std::vector<std::string> allArgsBuff;
        while (std::getline(gin, argbuff))
        {   if (argbuff == "#") //select a random argument
            {   std::mt19937 rn;
                try
                {   std::random_device rd;
                    rn.seed(rd());
                }
                catch(...)
                { }
                std::uniform_int_distribution<int> distrib(0, allArgsBuff.size() - 1);
                additionalArr = allArgsBuff[distrib(rn)];
                break;
            }
            else if (arg != "" && arg == argbuff) //we found the required argument
            {   additionalArr = argbuff;
                break;
            }
            else //add as candidate for random selection later
            {   allArgsBuff.push_back(argbuff);
            }
        }
    }

    return additionalArr;
}
