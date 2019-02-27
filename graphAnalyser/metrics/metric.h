#ifndef METRIC_H
#define METRIC_H

#include <string>
#include "graph.h"

class Metric {
    public:
        Metric(std::string name) : name(name) {
        }
        virtual ~Metric() {}
        virtual double calculate(const Graph& graph) const = 0;
        const std::string name;
};
#endif
