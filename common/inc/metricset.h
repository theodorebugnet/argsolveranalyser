#ifndef METRICSET_H
#define METRICSET_H

#include <map>
#include <fstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;


class MetricSet {
    public:
        MetricSet(fs::path);
        bool hasScore(std::string) const;
        int getScore(std::string) const;
        void setScore(std::string, int);
        void save() const;
        void clear();
    private:
        fs::path scoreFilePath;
        std::map<std::string, int> metricScores;
};

#endif
