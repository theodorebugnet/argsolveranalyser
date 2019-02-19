#include "metric.h"

class SparsenessMetric : public Metric {
    public:
        SparsenessMetric() :Metric("Sparseness") {
        }
        double calculate(const Graph& graph) const {
            int argCount = graph.getArgs().size();
            double attCount = graph.getAttacks().size(); //has to be a double to avoid rounding during calculation
            return attCount / (argCount * argCount);
        };
};
