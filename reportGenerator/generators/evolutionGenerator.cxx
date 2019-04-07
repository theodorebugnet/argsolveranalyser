#include <iomanip>
#include "generator.h"

class EvolutionGenerator : public Generator {
public:
    EvolutionGenerator() :Generator("evolution", "Provide a report of how solver performance evolved over time, given a set of timestamped benchmark runs.")
    { }

    void run()
    {
        Generator::run();

        if (!ok)
        {   return;
        }

        //first organise the data
        std::map<ch::system_clock::time_point, std::vector<ResultPoint*>> allResultsByDate;
        std::map<std::string, std::map<ch::system_clock::time_point, std::map<std::string, std::vector<ResultPoint*>>>> resultsByProblemAndDateAndGraph;
        std::map<std::string, int> graphCountAcrossDates;

        for (ResultPoint& pt : benchScores)
        {   allResultsByDate[pt.date].push_back(&pt);
            resultsByProblemAndDateAndGraph[pt.problem][pt.date][pt.graph].push_back(&pt);

            if (graphCountAcrossDates.find(pt.graph) == graphCountAcrossDates.end())
            {   graphCountAcrossDates[pt.graph] = 0;
            }
            graphCountAcrossDates[pt.graph] = graphCountAcrossDates[pt.graph] + 1;
        }

        // Next identify the set of graphs we can use across the dates
        // This doesn't necessarily identify the maximal set that can yield useful results,
        // as that would need a heuristic establishing the relative costs of skipping dates vs.
        // having a smaller graph sample, and be more complex. For a situation
        // where the same set of graphs is used for repeated benchmarking,
        // this is more than good enough
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

        //build per-problem sets of historic performance
        std::map<std::string, std::vector<std::pair<ch::system_clock::time_point, double>>> performanceGraphPerProblem;

        for (auto& pairIteratingProblems : resultsByProblemAndDateAndGraph)
        {   for (auto& pairIteratingDates : pairIteratingProblems.second)
            {   double totalScore = 0;
                for (const std::string& graph : frequentGraphsUniformSet)
                {   for (const ResultPoint* rpt : pairIteratingDates.second[graph])
                    {   double graphScore = rpt->time;
                        if (rpt->aborted)
                        {   graphScore *= 2;
                        }
                        totalScore += graphScore;
                    }
                }
                performanceGraphPerProblem[pairIteratingProblems.first].push_back(std::make_pair(pairIteratingDates.first, totalScore));
            }
        }

        // and now output them in gnuplot-compatible format
        for (const auto& problempair : performanceGraphPerProblem)
        {   const auto& problemData = problempair.second;
            std::cout << problempair.first << std::endl;
            for (const auto& problemDataPoint : problemData)
            {   std::cout << date::format("%FT%TZ", problemDataPoint.first) << " " << problemDataPoint.second << std::endl;
            }
            std::cout << "\n\n";
        }
    };
};
