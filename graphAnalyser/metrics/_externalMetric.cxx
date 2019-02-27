#include <exception>
#include <stdexcept>
#include "metric.h"
#include "pstream.h"

namespace fs = std::filesystem;

class ExternalMetric : public Metric {
    public:
        ExternalMetric(std::string name, std::string binPath) : Metric(name), binPath(binPath) {
        }
        double calculate(const Graph& graph) const {
            std::string arg = graph.fname();
            std::vector<std::string> argv {binPath, arg};
            redi::ipstream in(binPath, argv);

            std::string out;
            std::getline(in, out);
            in.close();
            int err, status;
            if ((err = in.rdbuf()->error()) == 0 && in.rdbuf()->exited()) {
                status = in.rdbuf()->status();
            } else {
                throw std::runtime_error("Error executing external metric " + err);
            }
            if (status != 0) {
                throw std::logic_error("Metric binary exited with non-zero status: " + status);
            }
            return std::stod(out);
        };
    private:
        std::string binPath;
};
