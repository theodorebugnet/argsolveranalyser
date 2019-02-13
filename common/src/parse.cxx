#include <fstream>
#include <string>
#include <iostream>
#include "graph.h"
#include "parse.h"

Graph* parseFile(std::string path) {
    Graph* ret = new Graph();

    std::ifstream problemFile(path);
    std::string tokenbuff, secondbuff;

    enum States : bool { arguments, attacks };
    States state = arguments;
    for (int ctr = 0; problemFile >> tokenbuff; ++ctr) {
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
    return ret;
}

