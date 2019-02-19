#include "metric.h"

class SizeMetric : public Metric {
    public:
        SizeMetric() :Metric("Size") {
        }
        double calculate(const Graph& graph) const {
            return graph.getArgs().size();
        };
};
