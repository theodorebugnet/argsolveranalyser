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

const std::vector<std::shared_ptr<Argument>>& Graph::getArgs() const {
    return args;
}

const std::vector<std::pair<std::shared_ptr<Argument>, std::shared_ptr<Argument>>>& Graph::getAttacks() const {
    return attacks;
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

void Graph::addArgument(std::shared_ptr<Argument> argument) {
    args.push_back(argument);
}

void Graph::addArgument(std::string name, int id) {
    args.push_back(std::shared_ptr<Argument>(new Argument(name, id)));
}

void Graph::addAttack(std::shared_ptr<Argument> source, std::shared_ptr<Argument> destination) {
    attacks.push_back(std::make_pair(source, destination));
}

/* //see header - unusable performance, dubious utility
void Graph::addAttack(std::string source, std::string destination) {
    auto src = std::find_if(args.begin(), args.end(), [&source](auto ptr) { return *ptr == source; });
    auto dest = std::find_if(args.begin(), args.end(), [&destination](std::shared_ptr<Argument> ptr) { return *ptr == destination; });
    attacks.push_back(std::make_pair(*src, *dest));
}
//*/
