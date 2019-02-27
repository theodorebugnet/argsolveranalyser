#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include <set>
#include <algorithm>
#include <filesystem>
#include "graph.h"
#include "util.h"
#include "SpookyV2.h"
#include "opts.h"

namespace fs = std::filesystem;

constexpr uint64_t seed1 = 1;
constexpr uint64_t seed2 = 2;

std::vector<std::string> get_graphlist() {
    std::vector<std::string> ret;
    bool useStore = true;
    std::vector<std::string> tmpGraphFiles;
    if (!opts["graphs"].empty()) {
        useStore = false;
        tmpGraphFiles = opts["graphs"].as<std::vector<std::string>>();
        for (std::string tmpgf : tmpGraphFiles) {
            ret.push_back(tmpgf);
        }
    }
    std::vector<std::string> graphDirs;
    if (!opts["graph-dirs"].empty()) {
        useStore = false;
        graphDirs = opts["graph-dirs"].as<std::vector<std::string>>();
    }
    if (useStore || opts["use-store"].as<bool>()) {
        if (!opts["store-path"].empty()) {
            graphDirs.push_back(opts["store-path"].as<std::string>() + "/graphs/");
        }
    }
    for (std::string dir : graphDirs) {
        for (auto& dirent : fs::recursive_directory_iterator(dir, fs::directory_options::follow_directory_symlink)) {
            if (dirent.is_regular_file()) {
                ret.push_back(dirent.path().string());;
            }
        }
    }

    return ret;
}

std::set<std::string> get_graphset() {
    std::set<std::string> ret;
    std::vector<std::string> tmp = get_graphlist();
    for (auto& g : tmp) {
        ret.insert(g);
    }
    return ret;
}

Graph* parseFile(std::string path) {
    std::set<std::string> args;
    std::set<std::pair<std::string, std::string>> atts;
    std::ifstream ifile(path);
    std::string tokenbuff, secondbuff;
    enum States : bool { arguments, attacks };
    States state = arguments;

    for (int ctr = 0; ifile >> tokenbuff; ctr++) {
        switch (state) {
            case arguments:
                if (tokenbuff == "#") state = attacks;
                else {
                    std::pair<std::set<std::string>::iterator, bool> insert = args.insert(tokenbuff);
                    if (!insert.second) {
                        std::cerr << "WARNING: Malformed tgf file: duplicate argument on line " << ctr << ". Ignoring." << std::endl;
                    }
                }
                break;
            case attacks:
                if (!(ifile >> secondbuff)) {
                    std::cerr << "ERROR: Malformed tgf file: an attack is missing its destination" << std::endl;
                    return nullptr;
                }
                std::pair<std::set<std::pair<std::string, std::string>>::iterator, bool> insert = atts.insert(std::make_pair(tokenbuff, secondbuff));
                if (!insert.second) {
                    std::cerr << "WARNING: Malformed tgf file: duplicate attack on line " << ctr << ". Ignoring." << std::endl;
                }
                break;
        }
    }

    Graph* ret = new Graph(path);
    SpookyHash graphHash;
    graphHash.Init(seed1, seed2);
    int i = 0;
    for (std::string arg : args) {
        graphHash.Update(arg.c_str(), arg.length());
        ret->addArgument(arg, i++);
    }
    for (std::pair<std::string, std::string> att : atts) {
        graphHash.Update((att.first + att.second).c_str(), (att.first + att.second).length());
        ret->addAttack(att.first, att.second);
    }

    uint64_t hash1;
    uint64_t hash2;
    graphHash.Final(&hash1, &hash2);
    std::stringstream streamtmp;
    streamtmp << std::hex << hash1 << hash2;
    ret->setHash(streamtmp.str());
    return ret;
}

