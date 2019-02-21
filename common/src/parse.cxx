#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include <set>
#include <algorithm>
#include "graph.h"
#include "parse.h"
#include "SpookyV2.h"

constexpr uint64_t seed1 = 1;
constexpr uint64_t seed2 = 2;

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

Graph* parseFile_old_baddedup(std::string path) {
    Graph* ret = new Graph(path);
    std::ifstream problemFile(path);
    std::string tokenbuff, secondbuff;
    SpookyHash graphHash;
    graphHash.Init(seed1, seed2);
    enum States : bool { arguments, attacks };
    States state = arguments;
    for (int ctr = 0; problemFile >> tokenbuff; ++ctr) {
        switch (state) {
            case arguments:
                graphHash.Update(tokenbuff.c_str(), tokenbuff.length());
                if (tokenbuff == "#") {
                    state = attacks;
                } else {
                    Argument* arg = new Argument(tokenbuff, ctr);
                    ret->addArgument(arg);
                }
                break;
            case attacks:
                if (!(problemFile >> secondbuff)) {
                    std::cerr << "Malformed tgf file: no attack destination on line " << ctr << std::endl;
                    return nullptr;
                } else {
                    graphHash.Update((tokenbuff + secondbuff).c_str(), (tokenbuff + secondbuff).length());
                    ret->addAttack(tokenbuff, secondbuff);
                }
                break;
        }
    }
    uint64_t hash1;
    uint64_t hash2;
    graphHash.Final(&hash1, &hash2);
    std::stringstream streamtmp;
    streamtmp << std::hex << hash1 << hash2;
    ret->setHash(streamtmp.str());
    return ret;
}

