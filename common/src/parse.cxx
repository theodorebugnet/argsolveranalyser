#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include "graph.h"
#include "parse.h"
#include "SpookyV2.h"

constexpr uint64_t seed1 = 1;
constexpr uint64_t seed2 = 2;

Graph* parseFile(std::string path) {
    Graph* ret = new Graph(path);

    std::ifstream problemFile(path);
    std::string tokenbuff, secondbuff;
    SpookyHash graphHash;
    graphHash.Init(seed1, seed2);
    enum States : bool { arguments, attacks };
    States state = arguments;
    const char* hashbuff;

    for (int ctr = 0; problemFile >> tokenbuff; ++ctr) {
        switch (state) {
            case arguments:
                hashbuff = tokenbuff.c_str();
                graphHash.Update(hashbuff, tokenbuff.length());
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
                    hashbuff = (tokenbuff + secondbuff).c_str();
                    graphHash.Update(hashbuff, tokenbuff.length());
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

