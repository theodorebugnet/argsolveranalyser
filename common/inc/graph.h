#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <utility>
#include <map>
#include <string>
#include <memory>

struct Argument {
    std::string name;
    int id;
    Argument(std::string name, int id) : name(name), id(id) {
    }

    bool operator==(const std::string& other) const {
        return other == name;
    }
    bool operator==(const int& other) const {
        return other == id;
    }
    bool operator==(const Argument& other) const {
        return other.id == id;
    }
    bool operator!=(const Argument& other) const {
        return !(*this == other);
    }

    bool operator<(const Argument& other) const {
        return id < other.id;
    }
    bool operator>(const Argument& other) const {
        return other < *this;
    }
    bool operator<=(const Argument& other) const {
        return !(*this > other);
    }
    bool operator>=(const Argument& other) const {
        return !(*this < other);
    }
};

class Graph {
    public:
        Graph(std::string);
        Graph() {};
        void addArgument(std::shared_ptr<Argument>);
        void addArgument(std::string, int);
        void printArgs() const;
        void printAttacks() const;
        void printOut() const;
        void setHash(std::string);
        const std::vector<std::shared_ptr<Argument>>& getArgs() const;
        const std::vector<std::pair<std::shared_ptr<Argument>, std::shared_ptr<Argument>>>& getAttacks() const;
        void addAttack(std::shared_ptr<Argument>, std::shared_ptr<Argument>);
        //void addAttack(std::string, std::string); //currently has abysmal performance - not exposed pending rewrite, which is deferred since it's not clear this overload is actually useful
        std::string hash() const;
        std::string fname() const;
    private:
        std::string hashval;
        std::string infname;
        std::vector<std::shared_ptr<Argument>> args;
        std::vector<std::pair<std::shared_ptr<Argument>, std::shared_ptr<Argument>>> attacks;
};

#endif
