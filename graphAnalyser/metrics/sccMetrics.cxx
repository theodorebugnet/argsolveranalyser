#include <functional>
#include "metric.h"

namespace //this is actually useless - need refactoring
{
    std::string graphHash;

    int index = 0;
    struct ArgAugment {
        int lowlink;
        int index;
        bool on_stack;
    };

    typedef std::vector<std::pair<std::shared_ptr<Argument>, std::shared_ptr<Argument>>> AttacksSet;
    typedef std::map<std::shared_ptr<Argument>, std::vector<std::shared_ptr<Argument>>> AttacksMap;
    AttacksMap atts;

    std::map<std::shared_ptr<Argument>, ArgAugment> augmentedArgs;
    typedef std::pair<const std::shared_ptr<Argument>, ArgAugment> AugmentedArg;

    std::vector<std::reference_wrapper<AugmentedArg>> stack;

    typedef std::vector<std::vector<AugmentedArg>> SCCSet;
    SCCSet SCCs;

    AttacksMap constructAttacksMap(const AttacksSet& attsIn)
    {
        AttacksMap ret;
        for (auto argPair : attsIn)
        {   ret[argPair.first].push_back(argPair.second);
        }
        return std::move(ret);
    }
    void augmentArgs(const std::vector<std::shared_ptr<Argument>>& argsIn)
    {
        augmentedArgs.clear();
        for (std::shared_ptr<Argument> argptr : argsIn)
        {   ArgAugment augmentation  = { -1, -1, false};
            augmentedArgs[argptr] = augmentation;
        }
    }

    void strongConnect(AugmentedArg& argin)
    {
        argin.second.index = index;
        argin.second.lowlink = index;
        ++index;
        stack.push_back(argin);
        argin.second.on_stack = true;
        for (std::shared_ptr<Argument>& target : atts[argin.first])
        {   AugmentedArg& augTarget = *(augmentedArgs.find(target));
            if (augTarget.second.index == -1)
            {   strongConnect(augTarget);
                argin.second.lowlink = std::min(argin.second.lowlink, augTarget.second.lowlink);
            }
            else if (augTarget.second.on_stack)
            {   argin.second.lowlink = std::min(argin.second.lowlink, augTarget.second.index);
            }
        }

        if (argin.second.lowlink == argin.second.index)
        {   std::vector<AugmentedArg> newSCC;
            AugmentedArg* top;
            do
            {   top = &(stack.back().get());
                stack.pop_back();
                top->second.on_stack = false;
                newSCC.push_back(*top);
            } while (top->first != argin.first); //this saves having to define operator==, and since it's a map .first is enough to uniquely identify
            SCCs.push_back(newSCC);
        }
    }

    void getSCCs(const Graph& graph)
    {
        index = 0;
        atts = constructAttacksMap(graph.getAttacks());
        augmentArgs(graph.getArgs());
        for (AugmentedArg& arg : augmentedArgs)
        {   if (arg.second.index == -1)
            {   strongConnect(arg);
            }
        }
    }
}

class SCCCountMetric : public Metric
{
    public:
        SCCCountMetric() : Metric("SCC-Count")
        { }
        double calculate(const Graph& graph) const
        {
            if (graphHash != graph.hash()) //if it's the same graph, the SCCs must have already been calculated
            {   SCCs.clear();
                getSCCs(graph);
                graphHash = graph.hash();
            }
            return SCCs.size();
        };
};

class SCCDensityMetric : public Metric
{
    public:
        SCCDensityMetric() : Metric("SCC-Density")
        { }
        double calculate(const Graph& graph) const
        {
            if (graphHash != graph.hash())
            {   SCCs.clear();
                getSCCs(graph);
                graphHash = graph.hash();
            }

            if (graph.getArgs().size() == 1) //special case, to avoid dividing by 0
            {   return 1;
            }

            double logsum = 0;
            for (std::vector<AugmentedArg>& scc : SCCs)
            {   logsum += std::log(scc.size());
            }

            return (float)(logsum / SCCs.size() / std::log(graph.getArgs().size())); //float gives enough precision and avoids weird issues with parsing at extreme values
        }
};
