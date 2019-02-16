#include <vector>
#include <utility>
#include <map>
#include <iostream>
#include <algorithm>
#include <memory>
#include "graph.h"

Graph::Graph(std::string infname) : infname(infname) {}

void Graph::setHash(std::string hash) {
    hashval = hash;
}

std::string Graph::fname() const {
    return infname;
}

std::string Graph::hash() const {
    return hashval;
}

void Graph::printArgs() const {
    for(auto a : args) {
        std::cout << a->name << ", ";
    }
    std::cout << std::endl;
}

void Graph::printAttacks() const {
    for (auto attack : attacks) {
        std::cout << attack.first->name << " attacks " << attack.second->name << std::endl;
    }
}

void Graph::printOut() const {
    this->printArgs();
    this->printAttacks();
}

void Graph::addArgument(Argument* argument) {
    args.push_back(std::shared_ptr<Argument>(argument));
}

void Graph::addAttack(Argument* source, Argument* destination) {
    attacks.push_back(std::make_pair(std::shared_ptr<Argument>(source), std::shared_ptr<Argument>(destination)));
}

void Graph::addAttack(std::string source, std::string destination) {
    auto src = std::find_if(args.begin(), args.end(), [&source](auto ptr) { return *ptr == source; });
    auto dest = std::find_if(args.begin(), args.end(), [&destination](std::shared_ptr<Argument> ptr) { return *ptr == destination; });
    attacks.push_back(std::make_pair(*src, *dest));
}
