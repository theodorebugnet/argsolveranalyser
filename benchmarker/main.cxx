#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include <set>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdio>
#include <sys/stat.h>
#include <fcntl.h>
#include <chrono>
#include "util.h"
#include "graph.h"
#include "opts.h"
#include "graphhashset.h"
#include "persistentargs.h"
#include "pstream.h"
#include "date.h"

#ifndef CONF_PATH
    #define CONF_PATH "./mapper.coonf"
#endif
#ifndef BIN_PATH
    #define BIN_PATH "./"
#endif

namespace po = boost::program_options;
namespace fs = std::filesystem;

const int magic_number = 143; //randomly selected, used as special return value

unsigned long getSizeInBytes(std::string sizeWithSuffix)
{
    unsigned long ret = 0;
    try
    {   size_t pos;
        ret = std::stoul(sizeWithSuffix, &pos);
        if (pos == sizeWithSuffix.size()) //no suffix
        {   ret *= (1024 * 1024); //MiB
        }
        else if (pos < sizeWithSuffix.size() - 1) //error
        {   std::cerr << "WARNING: Invalid size passed as --save-max-size: " << sizeWithSuffix << ". Defaulting to unlimited." << std::endl;
            ret = 0;
        }
        else
        {   switch (sizeWithSuffix.back())
            {   case 'G':
                    ret *= 1024;
                    [[fallthrough]];
                case 'M':
                    ret *= 1024;
                    [[fallthrough]];
                case 'k':
                    ret *= 1024;
                    [[fallthrough]];
                case 'b':
                    break;
                default:
                    std::cerr << "WARNING: Invalid suffix in option --save-max-size: " << sizeWithSuffix << ". Defaulting to unlimited." << std::endl;
                    ret = 0;
                    break;
            }
        }
    }
    catch (std::exception& e)
    {   std::cerr << "WARNING: Invalid size passed as --save-max-size: " << sizeWithSuffix << ". Defaulting to unlimited." << std::endl;
        ret = 0;
    }
    return ret;
}

int main(int argc, char** argv)
{
    /******** Define configuration options ********/
    po::options_description cmdOnly("Command-line only options");
    addHelpAndConfOpts(cmdOnly, CONF_PATH);

    po::options_description allSrcs("All configuration");
    addGraphFileOpts(allSrcs);
    allSrcs.add_options()
        ("reference-solver,r", po::value<std::string>(), "A trusted correct solver, used to generate reference solutions for graphs when one isn't found.\n")
        ("solver-executable,s", po::value<std::string>(), "The path to the solver binary that will be invoked.\n")
        ("run-id,i", po::value<std::string>(), "A string identifying the current run, under which the results will be saved. Previous results under the same ID will be overwritten. The special value TIMESTAMP will use a unique timestamp to identify the run, useful for automated or routine runs. If this option is not present, the value \"default\" is used as the ID.\n")
        ("recover,R", po::bool_switch(), "If previous results under the same ID are found, do not overwrite them, but instead assume they are from the same run that was interrupted and skip them to continue with the other problems.\n")
        ("clobber,C", po::bool_switch(), "If previous results under the same ID are found, delete them entirely rather than simply overwriting any collisions.\n")
        ("save-all,a", po::bool_switch(), "All solutions generated by the benchmarked solver will be saved to disk. (By default, only (fully or partially) incorrect ones are saved.\n")
        ("save-none,n", po::bool_switch(), "None of the solutions generated during the benchmark, not even incorrect ones, will be saved to disk. Overrides --save-all.\n")
        ("save-max-size,m", po::value<std::string>(), "Maximum size below which solutions will be saved to disk. Defaults to mebibytes; suffixes \"b\", \"k\", \"M\" or \"G\" can be used for bytes kibibytes, mebibytes and gibibytes respectively. Applies to all solutions. Unlimited by default.\n")
        ("save-correct-max-size,M", po::value<std::string>(), "Maximum size below which fully correct solutions will be saved to disk. Must be lower than --save-max-size. Only applies if the --save-all option is also set. As above, defaults to MiB and suffixes can be used to determine units. Unlimited by default.\n")
        ("time-limit,t", po::value<int>(), "Timeout (in seconds).\n")
        ("memory-limit,T", po::value<int>(), "Soft limit for memory (in megabytes).\n")
        ("problems,p", po::value<std::vector<std::string>>()->composing()->multitoken(), "A list of problems to be solved on all input graphs.\n");
    addQuietVerboseOpts(allSrcs);

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
    {   std::cout << "Usage: benchmarker [options]" << std::endl;
        std::cout << "Command line options generally override config file options, except when they specify lists." << std::endl;
        std::cout << "List arguments get merged from both sources.\n" << std::endl;
        std::cout << "The probo specification is used to invoke solvers; in brief, that means the solver is called as follows:\n"
            << "    SOLVER_NAME -f <filename of graph> -fo tgf -p <problem> [-a <argument name>]\n"
            << "For decision problems which require an argument name, specify the problem with a colon and, optionally, an argument identifier. If the identifier is avalid integer number, it is treated as a position number (in the tgf file); otherwise, it is treated as an argument name.\n"
            << " - If a dash '-' follows the colon, a random argument will be used.\n"
            << " - If nothing follows the colon, a random argument will be used IF a random argument has not previously been selected for this graph, otherwise that cached argument will be used rather than selecting a new random one.\n"
            << " - If a non-numeric name follows the colon, but a graph doesn't have a matching argument, a random argument will be used for that graph.\n"
            << " - If a number is used as identifier but a graph has less arguments than that, the last argument will be used for that graph.\n"
            << "For instance, using probo problem names:\n"
            << "    \"--problems DC-PR:1\" will invoke solvers to decide, credulously, the acceptance of the first argument (as found in the tfg file), under preferred semantics.\n"
            << "    \"--problems DC-PR:1 DC-PR:5 DC-PR:2 DS-GR:10\" will, for every graph, invoke solvers to solve credulous acceptance for the first, fifth, and second arguments as found in the tgf file, under preferred semantic, and also the skeptical acceptance of the tenth argument under grounded semantics - but if any graph has less than 10 arguments, skeptical acceptance under grounded semantics will instead be tested on the last argument specified in the tgf file.\n"
            << "    \"--problems DC-PR:a5\" will invoke DC-PR with the argument that has id \"a5\" for any graph that has an argument with this ID defined, and use a random argument for any graphs which do not have an argument called \"a5\".\n"
            << "Note that the colon is the only thing controlling the presense of the \"-a <argument>\" parameter in the solver invocation; the actual format used to represent the problems is not restricted, and so \"DC\" and \"DS\" are not treated as special cases.\n" << std::endl;
        std::cout << cmdOpts << std::endl;
        return 0;
    }

    if (opts["problems"].empty())
    {   std::cout << "ERROR: No problems specified. Benchmark cannot be run. Terminating." << std::endl;
        return 1;
    }

    std::string storeDir;
    if (opts["store-path"].empty())
    {   std::cout << "ERROR: No store path specified; default path was unset. Please specify a store directory where benchmark results will be saved. Terminating." << std::endl;
    }
    else
    {   storeDir = opts["store-path"].as<std::string>();
    }
    
    std::string runId;
    if (opts["run-id"].empty())
    {   runId = "default";
    }
    else if (opts["run-id"].as<std::string>() == "TIMESTAMP")
    {   auto now = std::chrono::system_clock::now();
        //runId = std::chrono::format("%FT%TZ", std::chrono::floor<std::chrono::milliseconds>(now));
        //Well this is what I WOULD use if GCC supported P0355R7, but...
        runId = date::format("%FT%TZ", date::floor<std::chrono::milliseconds>(now));
    }
    else
    {   runId = opts["run-id"].as<std::string>();
    }

    bool quiet = opts["quiet"].as<bool>();
    bool verbose = opts["verbose"].as<bool>();
    bool useHashCache = opts["use-hash-cache"].as<bool>();
    bool recover = opts["recover"].as<bool>();
    bool clobber = opts["clobber"].as<bool>();
    bool saveAll = opts["save-all"].as<bool>();

    std::vector<std::string> problems = opts["problems"].as<std::vector<std::string>>();

    int timeLimit = -1;
    if (!opts["time-limit"].empty())
    {   timeLimit = opts["time-limit"].as<int>();
    }
    int memLimit = -1;
    if (!opts["memory-limit"].empty())
    {   memLimit = opts["memory-limit"].as<int>();
    }

    std::uintmax_t saveMaxSize = 0;
    if (!opts["save-max-size"].empty())
    {   std::string sms = opts["save-max-size"].as<std::string>();
        saveMaxSize = getSizeInBytes(sms);
    }
    std::uintmax_t saveCorrectMaxSize = 0;
    if (!opts["save-correct-max-size"].empty())
    {   std::string scms = opts["save-correct-max-size"].as<std::string>();
        saveCorrectMaxSize = getSizeInBytes(scms);
    }
    if (saveCorrectMaxSize > saveMaxSize)
    {   saveCorrectMaxSize = saveMaxSize;
    }

    std::string solverpath;
    if (!opts["solver-executable"].empty())
    {   solverpath = opts["solver-executable"].as<std::string>();
    }
    std::string referenceSolverpath;
    if (!opts["reference-solver"].empty())
    {   referenceSolverpath = opts["reference-solver"].as<std::string>();
    }
    if (solverpath == "" && referenceSolverpath == "")
    {   std::cout << "INFO: No solvers were specified. Terminating." << std::endl;
    }
    else if (solverpath == "" && !quiet)
    {   std::cout << "INFO: No solver binary specified. No benchmarks will be ran; the reference solver will be used to generate any solutions that don't already exist." << std::endl;
    }
    else if (referenceSolverpath == "" && verbose)
    {   std::cout << "INFO: No reference solver specified; benchmarks will only be ran against graph+problem combinations for which a reference solution has previously been generated." << std::endl;
    }

    fs::path solverexecpath(solverpath);
    fs::path runIdDir(storeDir + "/benchmarks/" + solverexecpath.filename().string() + "/" + runId + "/");
    if (clobber)
    {   if (verbose)
        {   std::cout << "INFO: Clobber was set, wiping directory " << runIdDir.string() << "!" << std::endl;
        }
        fs::remove_all(runIdDir);
    }

    std::set<std::string> graphFiles = get_graphset();
    GraphHashSet ghset;
    PersistentArgs argCache;
    //For each graph, parse it to get the hash, then loop over problems
        //For each problem:
            //If "recover" flag was set, check if there's already a result with this run ID; if there is, continue; if not, or recover isn't set:
            //Check if there's a solution. If not, and a reference solver was specified, invoke it; otherwise print a warning and skip.
            //Now that a solution exists, run the benchmarked solver. Get results. Depending on options, save to disk.
            //Compare results here, generate correctness, save to disk.

    /******** Loop over graphs ********/
    for (std::string graphFile : graphFiles)
    {   //First, get the hash of the graph
        if (!quiet)
        {   std::cout << "Starting to process graph " << graphFile << std::endl;
        }
        std::string currHash;
        if (useHashCache && ghset.exists(graphFile))
        {   currHash = ghset.getHash(graphFile);
        }
        else
        {   std::unique_ptr<Graph> graphPtr = std::unique_ptr<Graph>(parseFile(graphFile));
            if (!graphPtr)
            {   std::cerr << "ERROR: Error parsing graph file " << graphFile << ". Skipping." << std::endl;
                continue;
            }
            currHash = graphPtr->hash();
            ghset.setHash(graphFile, currHash);
            if (verbose)
            {   std::cout << "    Loaded graph " << graphFile << std::endl;
            }

        }

        //create output directory if necessary
        fs::path outdir = runIdDir / currHash;
        fs::path soldir(storeDir + "/bench-solutions/" + currHash);
        try
        {   fs::create_directories(outdir);
            fs::create_directories(soldir);
        }
        catch (fs::filesystem_error& e)
        {   std::cerr << "ERROR: Unable to create output directory to store results and/or solutions: "
                << outdir.string() << " or " << soldir.string() << ": " << e.what() << ". Skipping graph." << std::endl;
            continue;
        }

        //Now we can start looping over the problems
        for (std::string fullproblem : problems)
        {   fs::path outfp = outdir / (fullproblem + ".output");
            fs::path resfp = outdir / (fullproblem + ".stat");

            //If recovery flag has been set, skip anything that already has a solution
            if (recover)
            {   if (fs::exists(outfp) && fs::exists(resfp))
                {   if (verbose)
                    {   std::cout << "        INFO: Recovery mode is on and problem solution already exists." << std::endl;
                    }
                    continue;
                }
            }

            //check if problem has additional argument
            std::string problem = fullproblem;
            std::string additionalArg;
            size_t colonpos = fullproblem.find(":");
            if (colonpos != std::string::npos)
            {   problem = fullproblem.substr(0, colonpos);
                std::string arg = fullproblem.substr(colonpos + 1, fullproblem.size());
                if (arg == "" && argCache.exists(currHash, problem))
                {   additionalArg = argCache.getArg(currHash, problem);
                    if (verbose)
                    {   std::cout << "        INFO: Arg cache for problem: " << additionalArg << std::endl;
                    }
                }
                else
                {   additionalArg = getAdditionalArg(graphFile, arg);
                }
                if (arg == "" || arg == "-")
                {   argCache.setArg(currHash, problem, additionalArg);
                }
            }

            //check reference solution exists here else act accordingly
            fs::path solfp = (soldir / (problem + (additionalArg == ""? "" : ":" + additionalArg)));
            if (!fs::exists(solfp))
            {
                if (referenceSolverpath == "")
                {   if (!quiet)
                    {   std::cout << "INFO: No reference solver specified and no solution for graph " << graphFile
                            << " and problem " << fullproblem << "; skipping." << std::endl;
                        continue;
                    }
                }
                else if (!fs::exists(referenceSolverpath) || !(fs::is_regular_file(referenceSolverpath) || fs::is_symlink(referenceSolverpath)))
                {   std::cerr << "WARNING: Reference solver path does not point to a file. Double-check the filepath. Skipping graphs with no existing solutions." << std::endl;
                    continue;
                }
                else
                {   if (verbose)
                    {   std::cout << "    No solution exists; running reference solver..." << std::endl;
                    }

                    //setup output
                    int outfd = creat(solfp.c_str(), 0644);
                    if (outfd < 0)
                    {   std::cerr << "ERROR: Unable to open solution output file for writing. Skipping this problem." << std::endl;
                        perror(NULL);
                        continue;
                    }

                    int pid = fork();
                    if (pid < 0) //error
                    {   std::cerr << "ERROR: Failed forking to invoke reference solver on graph " << graphFile
                            << " and problem " << fullproblem << "; skipping." << std::endl;
                        continue;
                    }
                    else if (pid > 0) //parent
                    {   int status;
                        wait(&status);
                        if (!WIFEXITED(status) || (WIFEXITED(status) && WEXITSTATUS(status) == magic_number))
                        {   std::cerr << "ERROR: Unable to execute reference solver! Skipping graph." << std::endl;
                            if(!fs::remove(solfp))
                            {   std::cerr << "Could not remove solution file..." << std::endl;
                            }
                            continue;
                        }
                        else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
                        {   std::cerr << "WARNING: Reference solver exited with non-zero status: " << WEXITSTATUS(status) << std::endl;
                        }
                    }
                    else //child
                    {   if (dup2(outfd, STDOUT_FILENO) < 0)
                        {   std::cerr << "ERROR: Unable to redirect reference solver to solution file. Skipping this." << std::endl;
                            _exit(1);
                        }
                        std::vector<std::string> argvct { referenceSolverpath,
                                "-f", graphFile,
                                "-fo", "tgf",
                                "-p", problem
                            };
                        if (additionalArg != "")
                        {   argvct.insert(argvct.end(), { "-a", additionalArg});
                        }

                        const char** args = new const char* [argvct.size()];
                        for (size_t i = 0; i < argvct.size(); i++)
                        {   args[i] = argvct[i].c_str();
                        }
                        args[argvct.size()] = NULL;

                        execv(referenceSolverpath.c_str(), (char**)args);
                        return magic_number;
                    }
                }
            }

            //Now we can invoke runsolver! Provided we have a solver argument, fork, build the command line, and execve.
            if (solverpath == "")
            {   continue;
            }

            if (verbose)
            {   std::cout << "        Solving problem " << problem << "..." << std::endl;
            }

            int pid = fork();
            if (pid < 0) //error
            {   std::cerr << "ERROR: Failed fork for invoking solver. Skipping graph " << graphFile << " and problem " << fullproblem << "." << std::endl;
                continue;
            }
            else if (pid > 0) //parent
            {   int status;
                wait(&status);
                if (!WIFEXITED(status) || (WIFEXITED(status) && WEXITSTATUS(status) == magic_number))
                {   std::cerr << "FATAL ERROR: Failed to execute runsolver. Aborting!" << std::endl;
                    continue;
                }
                else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
                {   std::cerr << "WARNING: Runsolver exited with non-zero status. Skipping further processing." << std::endl;
                    continue;
                }
            }
            else //child
            {   const char* runsolver = BIN_PATH "/runsolver";
                std::vector<std::string> argvct { runsolver,
                    "-w", "/dev/null",
                    "-v", resfp.string(),
                    "-o", outfp.string()
                };

                if (timeLimit > 0)
                {   argvct.push_back("-C");
                    argvct.push_back(std::to_string(timeLimit));
                }
                if (memLimit > 0)
                {
                    argvct.push_back("-M");
                    argvct.push_back(std::to_string(memLimit));
                }

                argvct.insert(argvct.end(), { solverpath,
                        "-f", graphFile,
                        "-fo", "tgf",
                        "-p", problem
                        });
                if (additionalArg != "")
                {   argvct.insert(argvct.end(), { "-a", additionalArg});
                }

                const char** args = new const char* [argvct.size()];
                for (size_t i = 0; i < argvct.size(); i++)
                {   args[i] = argvct[i].c_str();
                }
                args[argvct.size()] = NULL;

                execv(runsolver, (char**)args);
                return magic_number;
            }

            //runsolver is done
            //now compare solutions
            bool is_correct;
            long total, correct, wrong;
            total = correct = wrong = 0;

            //First we check for decision problem output ("YES" or "NO") and compare manually
            std::ifstream outif(outfp.c_str());
            std::string firstWordBuff;
            char b;
            while (outif >> b) //here we extract just the first word
            {   if (!std::isalpha(b))
                {   break;
                }
                firstWordBuff += b;
            }
            if (firstWordBuff == "YES" || firstWordBuff == "NO")
            {   std::ifstream solif(solfp.c_str());
                std::string solWordBuff;
                std::getline(solif, solWordBuff);
                is_correct = (solWordBuff == firstWordBuff);
            }
            else //We need to compare extensions
            {   if (verbose) std::cout << "        Running compare-extensions..." << std::endl;
                std::string binPath = BIN_PATH "/compare-extensions";
                std::vector<std::string> argv {binPath, solfp, outfp};
                redi::ipstream in(binPath, argv);

                std::string status;
                std::getline(in, status);
                if (status != "OK" && status != "WRONG")
                {   std::cerr << "ERROR: Unable to verify solution " << outfp << " against master " << solfp
                    << ". No correctness report will be generated. Error message: " << status << std::endl;
                    in.close();
                    is_correct = false; //provide default
                }
                else
                {   is_correct = (status == "OK"? true : false);
                    std::string totalstr, correctstr, wrongstr;
                    std::getline(in, totalstr, ' '); std::getline(in, status);
                    std::getline(in, correctstr, ' '); std::getline(in, status);
                    std::getline(in, wrongstr, ' '); std::getline(in, status);
                    try
                    {   total = std::stol(totalstr);
                        correct = std::stol(correctstr);
                        wrong = std::stol(wrongstr);
                    }
                    catch (std::exception& e)
                    {   std::cerr << "ERROR: solution comparison did not return correct values for the number of correct, wrong, and total extensions." << std::endl;
                        total = correct = wrong = -1;
                    }
                    in.close();

                    int err, status = 0;
                    if ((err = in.rdbuf()->error()) == 0 && in.rdbuf()->exited()) {
                        status = in.rdbuf()->status();
                    } else {
                        std::cerr << "        WARNING: Error executing solution comparison: " << err << std::endl;
                    }
                    if (status != 0 && verbose) {
                        std::cerr << "        INFO: Solution comparison exit status: " << status << std::endl;
                    }
                }
            }

            if (verbose)
            {   std::cout << "        INFO: Correct: " << is_correct << "; total: " << total << "; correct: " << correct << "; wrong: " << wrong << "." << std::endl;
            }

            //save to .stat here
            std::ofstream statof(resfp, std::ios::app);
            statof << "ISCORRECT=" << (is_correct? "true" : "false" ) << "\n"
                << "TOTALEXTS=" << total << "\n"
                << "CORRECTEXTS=" << correct << "\n"
                << "WRONGEXTS=" << wrong << std::endl;

            //now cleanup results if they're too large
            if (is_correct)
            {   if(!saveAll || (saveCorrectMaxSize > 0 && fs::file_size(outfp) > saveCorrectMaxSize))
                {   if (verbose) std::cout << "    Removing (correct) solution as " << (saveAll? "it's above max size" : "--save-all was not passed") << std::endl;
                    fs::remove(outfp);
                }
            }
            else if (!is_correct && saveMaxSize > 0 && fs::file_size(outfp) > saveMaxSize)
            {   if (verbose) std::cout << "    Removing (incorrect) solution as it's above max size" << std::endl;
                fs::remove(outfp);
            }
        }
        std::cout << "    Done" << std::endl;
    }
    argCache.save();
    ghset.save();
}

