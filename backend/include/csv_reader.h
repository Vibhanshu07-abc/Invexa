#pragma once
#ifndef CSV_READER_H
#define CSV_READER_H

#include "item.h"
#include <string>
#include <vector>

class CSVReader {
public:
    explicit CSVReader(const std::string& filePath);

    ItemList load();
    const std::vector<std::string>& getErrors() const;
    int skippedRows() const;

private:
    std::string filePath_;
    std::vector<std::string> errors_;
    int skipped_;

    std::string trim(const std::string& s) const;
    bool isValidInteger(const std::string& s) const;
    bool isValidDouble(const std::string& s) const;
    std::vector<std::string> splitCSVLine(const std::string& line) const;
    std::string defaultName(int id) const;
    std::string defaultCategory(int id) const;
    std::string defaultLocation(int id) const;
    int defaultDemand(int id, int expected) const;
    void log(const std::string& msg) const;
};

#endif
