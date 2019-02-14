#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include "graph.h"
#include "parse.h"
#include "SpookyV2.h"

constexpr int seed1 = 0;
constexpr int seed2 = 0;

Graph* parseFile(std::string path) {
    Graph* ret = new Graph();

    std::ifstream problemFile(path);
    std::string tokenbuff, secondbuff;
    SpookyHash graphHash;
    graphHash.Init(0, 0);

    enum States : bool { arguments, attacks };
    States state = arguments;
    for (int ctr = 0; problemFile >> tokenbuff; ++ctr) {
        graphHash.Update(&tokenbuff, tokenbuff.length());
        switch (state) {
            case arguments:
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

