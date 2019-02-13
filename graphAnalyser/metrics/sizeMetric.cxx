#include <string>
#include "metric.h"

class SizeMetric : public Metric {
    public:
        SizeMetric() :Metric("Size") {
        }
        int calculate(Graph& graph) {
            return graph.getArgs().size();
        };
};
