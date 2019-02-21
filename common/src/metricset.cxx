#include <exception>
#include <stdexcept>
#include <iostream>
#include "metricset.h"

MetricSet::MetricSet(fs::path scorefile) : scoreFilePath(scorefile) {
    std::cout << "  ##MSET: loading from " << scorefile.string() << std::endl;
    if (!fs::exists(scorefile)) { //no scores yet
        std::cout << "      ##MSET: does not exist, returning empty set" << std::endl;
        return;
    }

    std::ifstream infile(scorefile.c_str());
    if (!infile) {
        throw std::runtime_error("Unable to open existing results file: " + scorefile.string());
    }

    for (std::string namebuff, scorebuff; std::getline(infile, namebuff, '=') && std::getline(infile, scorebuff);) {
        std::cout << "      ##MSET: loading in metric " << namebuff << " with score " << scorebuff << std::endl;
        metricScores[namebuff] = std::stod(scorebuff);
    }
    infile.close();
}

bool MetricSet::exists(std::string metricName) const {
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
    ofile.close();
}

void MetricSet::clear() {
    metricScores.clear();
}
