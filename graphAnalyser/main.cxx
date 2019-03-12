#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <exception>
#include <fstream>
#include <set>
#include <unordered_set>
#include "graph.h"
#include "metric.h"
#include "opts.h"
#include "sizeMetric.cxx"
#include "sparsenessMetric.cxx"
#include "util.h"
#include "metricset.h"
#include "_externalMetric.cxx"
#include "graphhashset.h"

#ifndef CONF_PATH
    #define CONF_PATH "./analyser.coonf"
#endif

namespace po = boost::program_options;
namespace fs = std::filesystem;

std::set<std::string> graphFiles;
std::vector<std::unique_ptr<Metric>> metrics;
bool dry_run;

int main(int argc, char** argv)
{
    /******** Preliminary list of metrics ********/
    metrics.push_back(std::make_unique<SizeMetric>());
    metrics.push_back(std::make_unique<SparsenessMetric>());

    /******** Define configuration options ********/
    //only for command line
    po::options_description cmdOnly("Command-line only options");
    addHelpAndConfOpts(cmdOnly, CONF_PATH);
    cmdOnly.add_options()
        ("list-metrics,l", "Print a list of available metrics and exit.");

    //for both cmdline and config file
    po::options_description allSrcs("All configuration");
    addGraphFileOpts(allSrcs);
    allSrcs.add_options()
        ("force-recalculate,f", po::bool_switch(), "When an existing results file is found, do not trust the existing results and recalculate all metrics, overwriting any conflicting values.")
        ("clobber", po::bool_switch(), "When an existing results file is found for a given graph, truncate it before writing the new results.\nThis is useful if some metric has been deprecated and is not used anymore, but still clogs up the results files.")
        ("metric-whitelist,w", po::value<std::vector<std::string>>()->multitoken()->composing(), "A whitelist of metrics to use. Only these will be used to process the graph, all others will be skipped.\n")
        ("metric-blacklist,b", po::value<std::vector<std::string>>()->multitoken()->composing(), "A blacklist of metrics to disable. These metrics will not be ran.\nTakes precedence over the whitelist: a metric present in both options will not be ran.")
        ("dry-run", po::bool_switch(), "Without doing any actual calculations, print out a list of graphs that would be used, and for every graph, which metrics would be ran.")
        ("verbose,V", po::bool_switch(), "Provide progress output for each graph and metric (by default, only for each graph is printed")
        ("quiet,q", po::bool_switch(), "Suppress all progress output, leaving only error and warning messages");

    po::options_description cmdOpts;
    cmdOpts.add(cmdOnly).add(allSrcs);

    //parse
    try
    {   po::store(po::command_line_parser(argc, argv).options(cmdOpts).run(), opts);
        po::notify(opts);

        if (!opts["noconf"].as<bool>())
        {   std::ifstream confFile(opts["conf"].as<std::string>());
            if (confFile)
            {   po::store(po::parse_config_file(confFile, allSrcs), opts);
                po::notify(opts);
            }
        }

    }
    catch (std::exception& e)
    {   std::cerr << e.what() << std::endl;
        std::cerr << "Terminating." << std::endl;
        return 1;
    }

    /******** Handle parsed options ********/
    if (opts.count("help"))
    {   std::cout << "Usage: graph_analyser [options]" << std::endl;
        std::cout << "Command line options generally override config file options, except when they specify lists." << std::endl;
        std::cout << "List arguments get merged from both sources." << std::endl;
        std::cout << std::endl << "EXAMPLE: graph_analyser -g ~/my-graphs/a.tgf ~/my-graphs/b.tgf -d ~/my-graph-folder -b slow-metric useless-metric --use-store" << std::endl;
        std::cout << "This will analyse a.tgf, b.tgf, anything present in my-graph-folder (and subdirectories of that), and all graphs from the default store. It will skip processing \"slow-metric\" and \"useless-metric\"." << std::endl;
        std::cout << std::endl << "EXAMPLE: graph_analyser -d <STORE_DIR>/graph_set_a/ <STORE_DIR>/graph_set_b/" << std::endl;
        std::cout << "Where <STORE_DIR> is your installation's default store location: this will, instead of processing the entire store, only traverse the subdirectories graph_set_a and graph_set_b." << std::endl;
        std::cout << std::endl;
        std::cout << cmdOpts << std::endl;
        return 0;
    }
    //list-metrics handled below, after external metrics have been loaded
    if (opts["dry-run"].as<bool>())
    {   dry_run = true;
    }

    /******** Construct external metrics ********/
    fs::perms any_exec = fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec;
    fs::path extmetricp(opts["store-path"].as<std::string>() + "/external-metrics");
    if (fs::is_directory(extmetricp))
    {   for (auto& dirent : fs::directory_iterator(extmetricp)) //subdirectories are not traversed
        {   if (!dirent.is_regular_file())
            {   continue;
            }
            fs::perms permissions = dirent.status().permissions();
            if ((permissions & any_exec) != fs::perms::none) //is executable
            {   fs::path path = dirent.path();
                metrics.push_back(std::make_unique<ExternalMetric>(path.stem(), path.string())); //metric name is just filename w/o extension
            }
        }
    }

    //handle last "print and exit" type option, now that we have the required list
    if (opts.count("list-metrics"))
    {   for (const std::unique_ptr<Metric>& m : metrics)
        {   std::cout << m->name << std::endl;
        }
        return 0;
    }

    /******** Construct list of metrics ********/
    std::unordered_set<std::string> whitelist;
    bool has_wl = false;
    bool has_bl = false;
    std::unordered_set<std::string> blacklist;
    if (!opts["metric-whitelist"].empty())
    {   std::vector<std::string> tmp = opts["metric-whitelist"].as<std::vector<std::string>>();
        whitelist.insert(tmp.begin(), tmp.end());
        has_wl = true;
    }
    if (!opts["metric-blacklist"].empty())
    {   std::vector<std::string> tmp = opts["metric-blacklist"].as<std::vector<std::string>>();
        blacklist.insert(tmp.begin(), tmp.end());
        has_bl = true;
    }

    if (has_bl)
    {   for (std::vector<std::unique_ptr<Metric>>::iterator it = metrics.begin(); it != metrics.end();)
        {   if (blacklist.erase((*it)->name) == 1) //if it's in the blacklist - and incidentally remove it from blacklist now that it's handled
            {   it = metrics.erase(it); //erase
            }
            else
            {   ++it; //else carry on
            }
        }

        //if anything remains, it didn't correspond to any metric, so print warning to user
        for (std::string unused_bl : blacklist)
        {   std::cout << "INFO: " << unused_bl << " is not an available metric, and therefore cannot be blacklisted." << std::endl;
        }
    }

    if (has_wl)
    {   for (std::vector<std::unique_ptr<Metric>>::iterator it = metrics.begin(); it != metrics.end();)
        {   if (whitelist.erase((*it)->name) == 1) //as before, if it's in whitelist, and btw remove from whitelist
            {   ++it; //carry on
            }
            else
            {   it = metrics.erase(it); //else, it's not in whitelist, so erase
            }
        }

        //if anything remains, it didn't correspond to any metric, so print warning to user
        for (std::string unused_wl : whitelist)
        {   std::cout << "INFO: " << unused_wl << " is not an available metric, and therefore cannot be whitelisted." << std::endl;
        }
    }


    /******** Construct list of graphs ********/
    graphFiles = get_graphset();

    /******** Loop over graphs: parse graph, run metrics, save results ********/

    //Ensure output directory is good
    fs::path outDir(opts["store-path"].as<std::string>() + "/graph-scores/");
    if (fs::exists(outDir))
    {   if (!fs::is_directory(outDir))
        {   std::cerr << "Malformed store: entry \"/graph-scores/\" is not a directory" << std::endl;
            return 1;
        }
    }
    else
    {   fs::create_directory(outDir);
    }

    bool forcecalc = opts["force-recalculate"].as<bool>(); //for ease of access
    bool clobber = opts["clobber"].as<bool>();
    bool quiet = opts["quiet"].as<bool>();
    bool verbose = opts["verbose"].as<bool>();
    bool useHashCache = opts["use-hash-cache"].as<bool>();

    GraphHashSet ghset;

    //loop over graphs
    for (std::string graphFile : graphFiles)
    {   std::string currentHash;
        Graph *graphPtr = nullptr;

        if (!quiet) {
            std::cout << "Starting to process graph " << graphFile << std::endl;
        }

        if (useHashCache && ghset.exists(graphFile))
        {   currentHash = ghset.getHash(graphFile);
        }
        else
        {   Graph* graphPtr = parseFile(graphFile);
            if (!graphPtr)
            {   std::cerr << "ERROR: Error parsing graph file: " << graphFile <<". Skipping." << std::endl;
                continue;
            }

            currentHash = graphPtr->hash();
            if (verbose)
            {   std::cout << "Successfully loaded graph " << graphFile << std::endl;
            }
            ghset.setHash(graphFile, currentHash);
        }

        fs::path ofp = outDir / currentHash;

        std::unique_ptr<MetricSet> mset_ptr;
        try
        {   mset_ptr = std::make_unique<MetricSet>(ofp);
        }
        catch (std::exception& e)
        {   std::cerr << "ERROR: Unable to load file of existing results: " << ofp.string() << " for graph: " << graphFile
                << ". Skipping graph; please check permissions, or delete the file if you do not wish to keep the existing results, then re-run the "
                << "analyser for this graph using the -g option." << std::endl;
            continue;
        }
        if (!mset_ptr)
        {   std::cerr << "DEBUG: This should never show up. mset_ptr is null despite having been constructed. Skipping graphs " << graphFile << std::endl;
            continue;
        }
        MetricSet& mset = *mset_ptr;

        if (clobber)
        {   mset.clear();
        }

        //actually do the calculations for every metric
        for (const auto& m : metrics)
        {   if (mset.exists(m->name) && !forcecalc) //skip if already calculated
            {   if (dry_run)
                {   std::cout << "    (dry run) skipping metric " << m->name << " as it already exists and -f wasn't set" << std::endl;
                }
                continue;
            }

            if (dry_run)
            {   std::cout << "    (dry run) Would run metric " << m->name << " here" << std::endl;
                if (!graphPtr)
                {   std::cout << "    (dry run) Would lazy load graph here" << std::endl;
                }
            }
            else
            {   if (!graphPtr)
                {   if (verbose)
                    {   std::cout << "    Lazy loading graph..." << std::endl;
                    }
                    graphPtr = parseFile(graphFile);
                    if (!graphPtr) {
                        std::cerr << "ERROR: Error parsing graph file: " << graphFile <<". Skipping." << std::endl;
                        break; //break out of the metric looping, since we can't load the graph
                    }
                }
                if (verbose)
                {   std::cout << "    Running metric " << m->name << std::endl;
                }
                try
                {   double score = m->calculate(*graphPtr);
                    mset.setScore(m->name, score);
                }
                catch (std::exception& e)
                {   std::cerr << "ERROR: Error calculating metric " << m->name << ": " << e.what() << ". Skipping this metric." << std::endl;
                }
            }
        }

        //we're done with the graph
        //also the breakout point if lazy loading a graph fails
        delete graphPtr;

        //finally save the output
        if (dry_run)
        {   std::cout << "DRY RUN: Finished processing graph. Would write results to " << ofp.string() << "." << std::endl << std::endl;
        }
        else
        {   try
            {   mset.save();
            }
            catch (std::exception& e)
            {   std::cerr << "ERROR: Failure trying to write output file " << ofp.string() << " for graph " << graphFile << ". Exception message:" << std::endl;
                std::cerr << e.what() << std::endl;
                std::cerr << "Skipping graph!" << std::endl;
            }
        }
    }

    /******** Finally save the hash cache ********/
    ghset.save();
}

