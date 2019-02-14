#include <string>
#include "metric.h"

class SizeMetric : public Metric {
    public:
        SizeMetric() :Metric("Size") {
        }
        int calculate(const Graph& graph) const {
            return graph.getArgs().size();
        };
};
