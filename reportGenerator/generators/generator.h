#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>
#include <chrono>
#include "opts.h"
#include "date.h"
#include "metricset.h"

namespace ch = std::chrono;

class Generator {
public:
    Generator(std::string name, std::string description) : name(name), description(description)
    { }
    virtual ~Generator() {}
    virtual void run() = 0;
    const std::string name;
    const std::string description;

protected:
    typedef std::map<std::string, MetricSet> ScoresSet;

    struct ResultPoint
    {
        ch::system_clock::time_point date;
        std::string graph;
        std::string problem;
        bool correct;
        bool aborted;
        unsigned long mem;
        double time;
        double correctRate;

        std::string string()
        {
            return std::string("Result point:") +
                "\n\tUnix timestamp: " + date::format("%FT%TZ", date) +
                "\n\tgraph: " + graph +
                "\n\tproblem: " + problem +
                "\n\tcorrect: " + (correct? "yes" : "no") +
                "\n\taborte: " + (aborted? "yes" : "no") +
                "\n\tmem: " + std::to_string(mem) +
                "\n\ttime: " + std::to_string(time) +
                "\n\tCorrectness: " + std::to_string(correctRate);
        }
    };
    struct PerformanceChange
    {
        double previousTotalTime;
        double totalTime;
        unsigned long previousTotalMem;
        unsigned long totalMem;
        double previousCorrectRate;
        double correctRate;

        std::string graph;
        std::string problem;

        std::string string()
        {
            return std::string("Graph ") + graph + ", problem " + problem + ". Prev time: " + std::to_string(previousTotalTime)
                + ", time: " + std::to_string(totalTime)
                + "; prev mem: " + std::to_string(previousTotalMem) + ", mem: " + std::to_string(totalMem)
                + "; prev correctRate: " + std::to_string(previousCorrectRate) + ", correctRate: " + std::to_string(correctRate);
        }
    };

    bool verbose = false;
    bool quiet = false;
    bool ok = true;
    std::string solver;

    ScoresSet graphScores;
    std::vector<ResultPoint> benchScores;
};

void Generator::run() //loads the set of results for individual generators to use
{
    verbose = opts["verbose"].as<bool>();
    quiet = opts["quiet"].as<bool>();
    std::string solver;
    if (opts.count("solver"))
    {   solver = opts["solver"].as<std::string>();
    }
    else
    {   std::cerr << "ERROR: Please specify a solver." << std::endl;
        ok = false;
        return;
    }

    fs::path benchdir(opts["store-path"].as<std::string>() + "/benchmarks/" + solver);
    if (!fs::is_directory(benchdir))
    {   std::cerr << "WARNING: No benchmark data for solver specified. Nothing to report on." << std::endl;
        ok = false;
        return;
    }

    //load the result set
    for (const fs::directory_entry& id_dir : fs::directory_iterator(benchdir))
    {   if (!id_dir.is_directory())
        {   continue;
        }
        ch::system_clock::time_point timestamp;
        std::istringstream ss(id_dir.path().filename().string());
        ss >> date::parse("%FT%TZ", timestamp);
        if (ch::system_clock::to_time_t(timestamp) == 0)
        {   continue;
        }

        for (const fs::directory_entry& hash_dir : fs::directory_iterator(id_dir))
        {   if (!hash_dir.is_directory())
            {   if (verbose) std::cerr << "INFO: Non-directory item located in " << id_dir.path().string() << ". No action will be taken but this is abnormal." << std::endl;
                continue;
            }
            std::string currHash = hash_dir.path().filename().string();
            if (graphScores.find(currHash) == graphScores.end())
            {   graphScores.insert({currHash, MetricSet(hash_dir.path())});
            }

            for (const fs::directory_entry& resfile : fs::directory_iterator(hash_dir))
            {   if (!resfile.is_regular_file())
                {   continue;
                }
                if (!(resfile.path().extension() == ".stat"))
                {   continue;
                }

                std::string problem = resfile.path().stem();
                std::ifstream resfs(resfile.path());
                std::string tokbuff;

                bool is_correct;
                bool timeout;
                bool memout;
                double time;
                unsigned long mem;
                unsigned long total;
                unsigned long correct;
                unsigned long wrong;
                while (resfs >> tokbuff)
                {   size_t eqpos = tokbuff.find('=');
                    if (eqpos == std::string::npos)
                    {   if (!quiet)
                        {   std::cerr << "WARNING: Invalid line in file " << resfile.path().string() << ": " << tokbuff << ". Ignoring this line." << std::endl;
                        }
                        continue;
                    }

                    std::string key = tokbuff.substr(0, eqpos);
                    std::string val = tokbuff.substr(eqpos + 1, tokbuff.size());
                    try
                    {
                        if (key == "CPUTIME")
                        {   time = std::stod(val);
                        }
                        else if (key == "MAXVM")
                        {   mem = std::stoul(val);
                        }
                        else if (key == "ISCORRECT")
                        {   if (val == "true") is_correct = true;
                            else is_correct = false;
                        }
                        else if (key == "TIMEOUT")
                        {   if (val == "true") timeout = true;
                            else timeout = false;
                        }
                        else if (key == "MEMOUT")
                        {   if (val == "true") memout = true;
                            else memout = false;
                        }
                        else if (key == "TOTALEXTS")
                        {   total = std::stoul(val);
                        }
                        else if (key == "CORRECTEXTS")
                        {   correct = std::stoul(val);
                        }
                        else if (key == "WRONGEXTS")
                        {   wrong = std::stoul(val);
                        }
                    }
                    catch (std::exception& e)
                    {   std::cerr << "ERROR: Invalid value in file " << resfile.path().string() << " for key " << key << ": "
                        << val << ". Error: " << e.what() << std::endl;
                    }
                } //while

                double rate =
                    (is_correct? 1 : //if it's correct, it's fully correct
                     (total == 0? 0 : //if it's not fully correct, BUT it's a decision problem (yes/no answer, 0 total extension), it's fully incorrect
                      (correct / (wrong + total)))); //otherwise, it's not quite correct, but it's not a yes/no answer either, so calculate rate
                //strip additional argument definition from problems
                size_t colonpos = problem.find(':');
                if (colonpos != std::string::npos)
                {   problem = problem.substr(0, colonpos);
                }

                ResultPoint resp = {
                    timestamp,
                    currHash,
                    problem,
                    is_correct,
                    timeout || memout,
                    mem,
                    time,
                    rate,
                };

                benchScores.push_back(resp);
            }
        }
    }

    std::sort(benchScores.begin(), benchScores.end(), [](const ResultPoint& a, const ResultPoint& b){ return a.date < b.date; });
}
#endif
