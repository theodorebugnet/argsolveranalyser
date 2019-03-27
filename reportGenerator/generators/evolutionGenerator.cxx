#include "generator.h"

class EvolutionGenerator : public Generator {
    public:
        EvolutionGenerator() :Generator("evolution", "Provide a report of how solver performance evolved over time, given a set of timestamped benchmark runs.")
        { }

        void run() const
        {
            std::string solver;
            if (opts.count("solver"))
            {   solver = opts["solver"].as<std::string>();
            }
            else
            {   std::cerr << "ERROR: Please specify a solver." << std::endl;
                return;
            }

            std::cout << "Running " << this->name << " with solver " << solver << "!" << std::endl;
        };
};
