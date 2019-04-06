#include <chrono>
#include <iomanip>
#include "generator.h"
#include "date.h"
#include "metricset.h"

namespace ch = std::chrono;

class EvolutionGenerator : public Generator {
    private:
        struct ResultPoint
        {
            ch::system_clock::time_point date;
            std::string graph;
            std::string problem;
            bool correct;
            bool aborted;
            unsigned long mem;
            double time;
            double wrongRate;

            std::string string()
            {
                return std::string("Result point:") +
                    "\n\tUnix timestamp: " + std::to_string(date.time_since_epoch().count()) +
                    "\n\tgraph: " + graph +
                    "\n\tproblem: " + problem +
                    "\n\tcorrect: " + (correct? "yes" : "no") +
                    "\n\taborte: " + (aborted? "yes" : "no") +
                    "\n\tmem: " + std::to_string(mem) +
                    "\n\ttime: " + std::to_string(time) +
                    "\n\tCorrectness: " + std::to_string(wrongRate);
            }
        };

        struct PerformanceChange
        {
            double previousTotalTime;
            double totalTime;
            unsigned long previousTotalMem;
            unsigned long totalMem;

            std::string string()
            {
                return std::string("Prev time: ") + std::to_string(previousTotalTime)
                    + ", time: " + std::to_string(totalTime)
                    + "; prev mem: " + std::to_string(previousTotalMem) + ", mem: " + std::to_string(totalMem);
            }
        };

        typedef std::map<std::string, MetricSet> ScoresSet;

    public:
        EvolutionGenerator() :Generator("evolution", "Provide a report of how solver performance evolved over time, given a set of timestamped benchmark runs.")
        { }

        void run() const
        {
            bool verbose = opts["verbose"].as<bool>();
            bool quiet = opts["quiet"].as<bool>();

            std::string solver;
            if (opts.count("solver"))
            {   solver = opts["solver"].as<std::string>();
            }
            else
            {   std::cerr << "ERROR: Please specify a solver." << std::endl;
                return;
            }

            std::cout << "Running " << this->name << " with solver " << solver << "!" << std::endl;

            //load the data
            //structure: [graph hash + problem = performance]
            //scan for correctness regressions - output those
            //then scan for performance changes
            //for significant performace changes (esp. regressions): group by problem (for every graph), or group by metric of graph (for every problem)
            //goup by problem is easy: group by metric is hard
            

            fs::path benchdir(opts["store-path"].as<std::string>() + "/benchmarks/" + solver);
            if (!fs::is_directory(benchdir))
            {   std::cerr << "WARNING: No benchmark data for solver specified. Nothing to report on." << std::endl;
                return;
            }
            else std::cout <<"Report here..." << std::endl;

            ScoresSet graphScores;
            std::vector<ResultPoint> benchScores;

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

            //Now we have loaded all the graphscores and resultpoints
            //Time to perform analysis
            typedef std::map<ch::system_clock::time_point, std::vector<ResultPoint*>> ResultSetByDate;
            ResultSetByDate allResultsByDate;
            std::map<std::string, ResultSetByDate> resultsByGraphAndDate;
            std::map<std::string, ResultSetByDate> resultsByProblemAndDate;
            std::map<std::string, std::map<ch::system_clock::time_point, std::map<std::string, std::vector<ResultPoint*>>>> resultsByProblemAndDateAndGraph;

            std::map<std::string, int> graphCountAcrossDates;

            for (ResultPoint& pt : benchScores)
            {   //std::cout << pt.string() << std::endl;
                allResultsByDate[pt.date].push_back(&pt);
                resultsByGraphAndDate[pt.graph][pt.date].push_back(&pt);
                resultsByProblemAndDate[pt.problem][pt.date].push_back(&pt);
                resultsByProblemAndDateAndGraph[pt.problem][pt.date][pt.graph].push_back(&pt);

                if (graphCountAcrossDates.find(pt.graph) == graphCountAcrossDates.end())
                {   graphCountAcrossDates[pt.graph] = 0;
                }
                graphCountAcrossDates[pt.graph] = graphCountAcrossDates[pt.graph] + 1;
            }

            std::string mostFrequentGraph;
            std::unordered_set<std::string> frequentGraphsUniformSet;
            int curr_max = 0; 
            for (const auto& el : graphCountAcrossDates)
            {   if (el.second >= curr_max)
                {   curr_max = el.second;
                    mostFrequentGraph = el.first;
                }
            }
            bool first_found = 0;
            for (const auto& el : allResultsByDate)
            {   if (std::find_if(el.second.begin(), el.second.end(),
                        [&mostFrequentGraph](const ResultPoint* pt){ return pt->graph == mostFrequentGraph; }) == el.second.end())
                {   continue;
                }

                if (!first_found)
                {   first_found = true;
                    for (const ResultPoint* pt : el.second)
                    {   frequentGraphsUniformSet.insert(pt->graph);
                    }
                }
                else
                {   for (const ResultPoint* pt : el.second)
                    {   if (std::find_if(el.second.begin(), el.second.end(), [&graph = pt->graph](const ResultPoint* pt){ return pt->graph == graph; }) == el.second.end())
                        {   frequentGraphsUniformSet.erase(pt->graph);
                        }
                    }
                }
            }

            std::cout << "Uniform graphs:" << std::endl;
            for (std::string graph : frequentGraphsUniformSet)
            {   std::cout << "    " << graph << std::endl;
            }

            //build per-problem sets of historic performance
            std::map<std::string, std::vector<std::pair<ch::system_clock::time_point, double>>> performanceGraphPerProblem;

            for (auto& pairIteratingProblems : resultsByProblemAndDateAndGraph)
            {   std::cout << pairIteratingProblems.first << std::endl;
                for (auto& pairIteratingDates : pairIteratingProblems.second)
                {   std::cout << "    " << date::format("%FT%TZ", pairIteratingDates.first) << std::endl;
                    double totalScore = 0;
                    for (const std::string& graph : frequentGraphsUniformSet)
                    {   std::cout << "        " << graph << std::endl;
                        for (const ResultPoint* rpt : pairIteratingDates.second[graph])
                        {   double graphScore = rpt->time;
                            if (rpt->aborted)
                            {   graphScore *= 2;
                            }
                            std::cout << "            " << graphScore << std::endl;
                            totalScore += graphScore;
                        }
                    }
                    performanceGraphPerProblem[pairIteratingProblems.first].push_back(std::make_pair(pairIteratingDates.first, totalScore));
                }
            }
/*
            for (const std::string& graph : frequentGraphsUniformSet)
            {   std::cout << graph << std::endl;
                const auto& resultByDateForGraph = resultsByGraphAndProblemAndDate[graph];
                for (const auto& pair : resultByDateForGraph)
                {   std::cout << "    " << pair.first << std::endl;
                    for (const auto& dateAndResultSetPair : pair.second)
                    {   std::cout << "        " << date::format("%FT%TZ", dateAndResultSetPair.first) << std::endl;
                        const std::vector<ResultPoint*> resvct = dateAndResultSetPair.second;
                        double totalTime = 0;
                        for (ResultPoint* pt : resvct)
                        {   performanceGraphPerProblem[pt->problem].push_back(std::make_pair(pt->date, 0));
                        }
                    }
                }
            }
//*/
            for (const auto& problempair : performanceGraphPerProblem)
            {   const auto& problemData = problempair.second;
                std::cout << problempair.first << std::endl;
                for (const auto& problemDataPoint : problemData)
                {   std::cout << date::format("%FT%TZ", problemDataPoint.first) << " " << problemDataPoint.second << std::endl;
                }
                std::cout << "\n\n";
            }

            //analyse per-graph results
            //std::map<std::string, std::vector<std::pair<ch::system_clock::time_point, PerformanceChange>>> performanceRegressionsByGraph;

            /*float significantRegression = 1.10; //if new result is more than 10% slower than old, flag
            for (const auto& elbgd : resultsByGraphAndDate) //for every graph...
            {   const ResultSetByDate& rsbd = elbgd.second;
                double lastTotalTime = -1;
                unsigned long lastTotalMem = 0;
                std::cout << "In hash " << elbgd.first << "..." << std::endl;
                for (const auto& elptv : rsbd) //go through the dates it was benchmarked. For every date...
                {   const std::vector<ResultPoint*> ptv = elptv.second;
                    double totalTime = 0;
                    unsigned long totalMem = 0;
                    std::cout << "    In date " << std::to_string(elptv.first.time_since_epoch().count()) << "..." << std::endl;
                    for (ResultPoint* pt : ptv)//...check every data point (in practice, every problem) and add up...
                    {   totalTime += pt->time;
                        totalMem += pt->mem;
                        std::cout << "        In problem " << pt->problem << "..." << std::endl;
                    }

                    if (lastTotalTime > 0 &&
                            (totalTime > (lastTotalTime * significantRegression) ||
                            totalMem > (lastTotalMem * significantRegression))) //then check if there was a performance regression
                    {   PerformanceChange pc = {
                            lastTotalTime,
                            totalTime,
                            lastTotalMem,
                            totalMem
                        };
                        std::cerr << "There is a regression! Total time: " << totalTime << ", lastTotalTime: " << lastTotalTime << ";"
                            << " Total mem: " << totalMem << ", lastTotalMem: " << lastTotalMem << std::endl;
                        //performanceRegressionsByGraph.insert(std::make_pair(elbgd.first, std::make_pair(elptv.first, pc))); //they all have the same date and graph anyway
                        performanceRegressionsByGraph[elbgd.first].push_back(std::make_pair(elptv.first, pc)); //they all have the same date and graph anyway
                    }

                    lastTotalTime = totalTime;
                    lastTotalMem = totalMem;
                }
            }
            std::cout << "Printing regressions by graph:" << std::endl;
            for (auto& graph : performanceRegressionsByGraph)
            {   for (auto& regression : graph.second)
                {   std::cout << "    " <<  graph.first << " " << date::format("%FT%TZ", regression.first) << " " << regression.second.string() << std::endl;
                }
            }*/
        };
};
