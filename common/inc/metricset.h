#ifndef METRICSET_H
#define METRICSET_H

#include <map>
#include <fstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;


class MetricSet
{
    public:
        MetricSet(fs::path);
        bool exists(std::string) const;
        double getScore(std::string) const;
        void setScore(std::string, double);
        void save() const;
        void clear();
    private:
        fs::path scoreFilePath;
        std::map<std::string, double> metricScores;
};

#endif
