#include "generator.h"

template<typename T> using SetByGraphAndProblem = std::map<std::string, std::map<std::string, T>>;

class RegressionGenerator : public Generator {
private:
    const float significantRegression = 1.2f; //minimum decrease in perfromance that will be flagged
    const float significantDeviation = 1.0f; //IQR multiplier, added to average, used for threshold for flagging metrics
public:
    RegressionGenerator() :Generator("regression", "Provide a report of how the latest solver changes impacted performance, highlighting any regressions or significant improvements.")
    { }

    void run()
    {
        Generator::run();

        std::map<ch::system_clock::time_point, SetByGraphAndProblem<ResultPoint*>> resultsByDateAndGraphAndProblem;

        std::vector<std::pair<std::string, std::string>> graphsAndProblemsToCheck;
        SetByGraphAndProblem<ResultPoint*> latestBench;
        std::set<std::string> allGraphsInLatestBench; //used in last step
        ch::system_clock::time_point latestDate;
        std::vector<PerformanceChange> performanceChanges;

        for (ResultPoint& pt : benchScores)
        {   resultsByDateAndGraphAndProblem[pt.date][pt.graph][pt.problem] = &pt;
        }

        bool first = true;
        for (auto rit = resultsByDateAndGraphAndProblem.rbegin(); rit != resultsByDateAndGraphAndProblem.rend(); rit++)
        {   //std::cout << date::format("%FT%TZ", rit->first) << std::endl;
            SetByGraphAndProblem<ResultPoint*> resultsForDate = rit->second;
            if (first)
            {   latestBench = resultsForDate;
                latestDate = rit->first;
                for (auto& pairByGraphAndProblem : resultsForDate)
                {   for (auto& pairOfProblemAndResults : pairByGraphAndProblem.second)
                    {   graphsAndProblemsToCheck.push_back({pairByGraphAndProblem.first, pairOfProblemAndResults.first});
                        allGraphsInLatestBench.insert(pairByGraphAndProblem.first);
                    }
                }
                first = false;
            }
            else
            {   if (graphsAndProblemsToCheck.empty())
                {   break;
                }

                for (auto it = graphsAndProblemsToCheck.begin(); it != graphsAndProblemsToCheck.end();)
                {   bool erased = false;
                    std::string graph = it->first;
                    std::string problem = it->second;
                    if (resultsForDate.find(graph) != resultsForDate.end())
                    {   std::map<std::string, ResultPoint*> resultsByProblem = resultsForDate[graph];
                        if (resultsByProblem.find(problem) != resultsByProblem.end())
                        {   ResultPoint* pt = resultsByProblem[problem];
                            ResultPoint* currentPt = latestBench[graph][problem];
                            PerformanceChange change {
                                pt->time,
                                currentPt->time,
                                pt->mem,
                                currentPt->mem,
                                pt->correctRate,
                                currentPt->correctRate,
                                graph,
                                problem,
                            };
                            performanceChanges.push_back(change);
                            it = graphsAndProblemsToCheck.erase(it);
                            erased = true;
                        }
                    }
                    if (!erased)
                    {   ++it;
                    }
                }
            }
        }

        //Now we have every data point in the latest benchmark correlated with the latest available previous result for the same graph and problem.
        //Time to identify regressions

        std::vector<PerformanceChange*> performanceRegressions;
        std::vector<PerformanceChange*> timeRegressions;
        std::vector<PerformanceChange*> memoryRegressions;
        std::vector<PerformanceChange*> correctnessRegressions;
        std::vector<PerformanceChange*> correctnessImprovements;

        for (PerformanceChange& change : performanceChanges)
        {   bool performancePushed = false;
            if (change.totalTime > (change.previousTotalTime * significantRegression))
            {   performanceRegressions.push_back(&change);
                performancePushed = true;
                timeRegressions.push_back(&change);
            }

            if (change.totalMem > (change.previousTotalMem * significantRegression))
            {   if (!performancePushed) performanceRegressions.push_back(&change);
                memoryRegressions.push_back(&change);
            }

            if (change.correctRate > change.previousCorrectRate)
            {   correctnessImprovements.push_back(&change);
            }

            if (change.correctRate < change.previousCorrectRate)
            {   correctnessRegressions.push_back(&change);
            }
        }

        //now output
        std::cout << date::format("%FT%TZ", latestDate) << std::endl;
        std::cout << "Time regressions: " << performanceRegressions.size() << "\n"
            << "Memory regressions: " << memoryRegressions.size() << "\n"
            << "Correctness regressions: " << correctnessRegressions.size() << "\n"
            << "Correctness improvements: " << correctnessImprovements.size() << "\n"
            << "No previous results: " << graphsAndProblemsToCheck.size() << "\n"
            << std::endl;

        int hashw = 35;
        int probw = 8;
        int oldw = 10;
        int neww = 10;
        if (!timeRegressions.empty())
        {   std::cout << "TIME REGRESSIONS" << std::endl;
            std::cout << std::left << std::setw(hashw) << "Graph hash" << " " << std::setw(probw) << "Problem" << " " << std::setw(oldw)
                << "Old value" << " " << std::setw(neww) << "New value" << std::endl;
            for(PerformanceChange* c : timeRegressions)
            {   std::cout << std::left << std::setw(hashw) << c->graph << " " << std::setw(probw) << c->problem
                    << " " << std::setw(oldw) << c->previousTotalTime << " " << std::setw(neww) << c->totalTime << std::endl;
            }
            std::cout << std::endl;
        }
        //horribly ugly code, needs refactoring
        if (!memoryRegressions.empty())
        {   std::cout << "MEMORY REGRESSIONS" << std::endl;
            std::cout << std::left << std::setw(hashw) << "Graph hash" << " " << std::setw(probw) << "Problem" << " " << std::setw(oldw)
                << "Old value" << " " << std::setw(neww) << "New value" << std::endl;
            for(PerformanceChange* c : memoryRegressions)
            {   std::cout << std::left << std::setw(hashw) << c->graph << " " << std::setw(probw) << c->problem
                    << " " << std::setw(oldw) << c->previousTotalMem << " " << std::setw(neww) << c->totalMem << std::endl;
            }
            std::cout << std::endl;
        }
        if (!correctnessRegressions.empty())
        {   std::cout << "CORRECTNESS REGRESSIONS" << std::endl;
            std::cout << std::left << std::setw(hashw) << "Graph hash" << " " << std::setw(probw) << "Problem" << " " << std::setw(oldw)
                << "Old value" << " " << std::setw(neww) << "New value" << std::endl;
            for(PerformanceChange* c : correctnessRegressions)
            {   std::cout << std::left << std::setw(hashw) << c->graph << " " << std::setw(probw) << c->problem
                    << " " << std::setw(oldw) << c->previousCorrectRate << " " << std::setw(neww) << c->correctRate << std::endl;
            }
            std::cout << std::endl;
        }
        if (!correctnessImprovements.empty())
        {   std::cout << "CORRECTNESS IMPROVEMENTS" << std::endl;
            std::cout << std::left << std::setw(hashw) << "Graph hash" << " " << std::setw(probw) << "Problem" << " " << std::setw(oldw)
                << "Old value" << " " << std::setw(neww) << "New value" << std::endl;
            for(PerformanceChange* c : correctnessImprovements)
            {   std::cout << std::left << std::setw(hashw) << c->graph << " " << std::setw(probw) << c->problem
                    << " " << std::setw(oldw) << c->previousCorrectRate << " " << std::setw(neww) << c->correctRate << std::endl;
            }
            std::cout << std::endl;
        }

        if (performanceRegressions.size() == 0)
        {   return;
        }

        std::cout << "###" << std::endl;

        if (performanceRegressions.size() >= allGraphsInLatestBench.size() - 0)
        {   std::cout << "NO ANALYSIS: Insufficient data size to compare graphs that suffered regressions to ones that didn't." << std::endl;
            return;
        }

        //TODO: loop through graphs which did not have a performance regression. Gather averages for every metric (and error rates?). Loop through the graphs which did have a performance regression, and see if any have metric scores outside the norm.
        std::map<std::string, std::pair<double, double>> metricAvAndIQR;
        std::map<std::string, MetricSet> msets;

        fs::path metricsDir(opts["store-path"].as<std::string>() + "/graph-scores/");

        std::vector<std::string> graphsWithNoRegressions;
        std::vector<std::string> graphsWithRegression;

        for (PerformanceChange* pc : performanceRegressions)
        {   graphsWithRegression.push_back(pc->graph);
        }
        for (std::string graph : allGraphsInLatestBench)
        {   if (std::find(graphsWithRegression.begin(), graphsWithRegression.end(), graph) == graphsWithRegression.end())
            {   graphsWithNoRegressions.push_back(graph);
            }

            MetricSet mset(metricsDir / graph);
            msets.insert({graph, mset});
        }

        std::map<std::string, std::vector<double>> metricRunningValues;
        for (std::string graph : graphsWithNoRegressions)
        {   MetricSet metrics = msets.find(graph)->second;
            double size = 1.0;
            if (metrics.exists("Size"))
            {   size = metrics.getScore("Size"); //crude normalisation
            }
            for (const auto& mpair : metrics.getAllScores())
            {   metricRunningValues[mpair.first].push_back(mpair.second / size);
            }
        }

        for (auto& mrvpair : metricRunningValues)
        {   std::vector<double>& vals = mrvpair.second;
            int valsize = vals.size();
            std::sort(vals.begin(), vals.end());

            double total = 0;
            for (double v : vals)
            {   total += v;
            }
            double av = total / valsize;
            int q1 = valsize/4;
            int q3 = valsize*3/4;
            double iqr = vals[q3] - vals[q1];
            metricAvAndIQR.insert({mrvpair.first, {av, iqr}});
        }

        std::vector<std::string> possibleCorrelations;
        for (std::string graph : graphsWithRegression)
        {   MetricSet metrics = msets.find(graph)->second;
            double size = 1.0;
            if (metrics.exists("Size"))
            {   size = metrics.getScore("Size");
            }
            for (const auto& mpair : metrics.getAllScores())
            {   const auto& avIqrPair = metricAvAndIQR.find(mpair.first)->second;
                double val = mpair.second / size;
                double av = avIqrPair.first;
                double iqr = avIqrPair.second * significantDeviation;
                if ((av + iqr) < val)
                {   possibleCorrelations.push_back(graph + ": " + mpair.first + ": was found to be significantly higher than average");
                }
                if ((av - iqr) > val)
                {   possibleCorrelations.push_back(graph + ": " + mpair.first + ": was found to be significantly lower than average");
                }
            }
        }

        if (!possibleCorrelations.empty())
        {   std::cout << "Among graphs which had performance regressions, the following metrics were found to be significantly different from the average among graphs which did not suffer from a regression:\n" << std::endl;
            for (std::string msg : possibleCorrelations)
            {   std::cout << msg << std::endl;
            }
        }
    };
};
