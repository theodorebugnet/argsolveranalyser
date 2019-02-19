#include <exception>
#include <stdexcept>
#include <iostream>
#include "metricset.h"

MetricSet::MetricSet(fs::path scorefile) : scoreFilePath(scorefile) {
    if (!fs::exists(scorefile)) { //no scores yet
        return;
    }

    std::ifstream infile(scorefile.c_str());
    if (!infile) {
        throw std::runtime_error("Unable to open existing results file: " + scorefile.string());
    }

    for (std::string namebuff, scorebuff; std::getline(infile, namebuff, '=') && std::getline(infile, scorebuff);) {
        metricScores[namebuff] = std::stod(scorebuff);
    }
}

bool MetricSet::hasScore(std::string metricName) const {
    return metricScores.find(metricName) != metricScores.end();
}

double MetricSet::getScore(std::string metricName) const {
    return metricScores.at(metricName);
}

void MetricSet::setScore(std::string metricName, double score) {
    metricScores[metricName] = score;
}

void MetricSet::save() const {
    std::ofstream ofile(scoreFilePath.c_str());
    if (!ofile) {
        throw std::runtime_error("Unable to open results file for writing: " + scoreFilePath.string());
    }

    for (auto& entry : metricScores) {
        ofile << entry.first << '=' << entry.second << std::endl;
    }
}

void MetricSet::clear() {
    metricScores.clear();
}
