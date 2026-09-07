#include "../include/csv_reader.h"
#include "../include/config_service.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <set>

using namespace std;

CSVReader::CSVReader(const string& filePath)
    : filePath_(filePath), skipped_(0)
{
    log("Initialized with file: " + filePath_);
}

void CSVReader::log(const string& msg) const {
    cout << "[CSV] " << msg << "\n";
}

string CSVReader::trim(const string& s) const {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool CSVReader::isValidInteger(const string& s) const {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    if (i == s.size()) return false;
    for (; i < s.size(); ++i) {
        if (!isdigit((unsigned char)s[i])) return false;
    }
    return true;
}

bool CSVReader::isValidDouble(const string& s) const {
    if (s.empty()) return false;
    size_t i = 0;
    int dots = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    if (i == s.size()) return false;
    for (; i < s.size(); ++i) {
        if (s[i] == '.') {
            dots++;
            if (dots > 1) return false;
        } else if (!isdigit((unsigned char)s[i])) {
            return false;
        }
    }
    return true;
}

vector<string> CSVReader::splitCSVLine(const string& line) const {
    vector<string> fields;
    string field;
    bool inQuotes = false;

    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            fields.push_back(trim(field));
            field.clear();
        } else {
            field += c;
        }
    }

    fields.push_back(trim(field));
    return fields;
}

string CSVReader::defaultName(int id) const {
    return "Item-" + to_string(id);
}

string CSVReader::defaultCategory(int id) const {
    const auto& categories = ConfigService::instance().data().defaultCategories;
    return categories[(id - 1) % categories.size()];
}

string CSVReader::defaultLocation(int id) const {
    const auto& zones = ConfigService::instance().data().defaultZones;
    return zones[(id - 1) % zones.size()];
}

int CSVReader::defaultDemand(int id, int expected) const {
    const auto& config = ConfigService::instance().data();
    int base = max(config.defaultDemandMinimum, expected / config.defaultDemandDivisor);
    return base + (id % 9) * 3;
}

ItemList CSVReader::load() {
    ItemList items;
    ifstream file(filePath_);

    if (!file.is_open()) {
        string err = "ERROR: Cannot open file: " + filePath_;
        errors_.push_back(err);
        log(err);
        return items;
    }

    string line;
    int lineNum = 0;
    set<int> seenIds;

    while (getline(file, line)) {
        lineNum++;
        line = trim(line);

        if (line.empty()) continue;

        if (lineNum == 1) {
            char first = line[0];
            if (isalpha((unsigned char)first) || first == '#') continue;
        }

        vector<string> fields = splitCSVLine(line);
        if (fields.size() != 4 && fields.size() != 11 && fields.size() != 13) {
            string err = "Line " + to_string(lineNum) + " has unsupported column count.";
            errors_.push_back(err);
            skipped_++;
            log(err);
            continue;
        }

        bool valid = true;
        if (!isValidInteger(fields[0])) valid = false;
        if (fields.size() == 4) {
            valid = valid &&
                    isValidInteger(fields[1]) &&
                    isValidInteger(fields[2]) &&
                    isValidDouble(fields[3]);
        } else {
            valid = valid &&
                    !fields[1].empty() &&
                    !fields[2].empty() &&
                    isValidInteger(fields[3]) &&
                    isValidInteger(fields[4]) &&
                    isValidDouble(fields[5]) &&
                    isValidInteger(fields[6]) &&
                    !fields[7].empty() &&
                    !fields[8].empty() &&
                    isValidInteger(fields[9]) &&
                    isValidInteger(fields[10]);
            if (fields.size() == 13) {
                valid = valid &&
                        isValidInteger(fields[11]) &&
                        !fields[12].empty();
            }
        }

        if (!valid) {
            string err = "Line " + to_string(lineNum) + " failed validation.";
            errors_.push_back(err);
            skipped_++;
            log(err);
            continue;
        }

        int id = stoi(fields[0]);
        if (id <= 0 || seenIds.count(id)) {
            string err = "Line " + to_string(lineNum) + " has invalid or duplicate id.";
            errors_.push_back(err);
            skipped_++;
            log(err);
            continue;
        }

        seenIds.insert(id);

        Item item;
        if (fields.size() == 4) {
            int expected = stoi(fields[1]);
            int actual = stoi(fields[2]);
            double price = stod(fields[3]);
            item = Item(id,
                        defaultName(id),
                        defaultCategory(id),
                        expected,
                        actual,
                        price,
                        defaultDemand(id, expected),
                        defaultLocation(id),
                        actual < expected ? defaultLocation(id) : defaultLocation(id));

            if (id % 4 == 0 && item.actual > 0) {
                item.currentLocation = defaultLocation((id % ConfigService::instance().data().alternateZoneModuloBase) + 1);
            }
            item.refreshDerived();
        } else {
            const int warehouseId = fields.size() == 13 ? stoi(fields[11]) : 1;
            const string warehouseName = fields.size() == 13 ? fields[12] : "Main Warehouse";
            item = Item(id,
                        fields[1],
                        fields[2],
                        stoi(fields[3]),
                        stoi(fields[4]),
                        stod(fields[5]),
                        stoi(fields[6]),
                        fields[7],
                        fields[8],
                        stoi(fields[10]),
                        warehouseId,
                        warehouseName);
            item.restockCost = stoi(fields[9]);
            item.refreshDerived();
        }

        items.push_back(item);
        log("Loaded item id=" + to_string(item.id) + " risk=" + item.riskLevel);
    }

    log("Items loaded: " + to_string(items.size()));
    return items;
}

const vector<string>& CSVReader::getErrors() const {
    return errors_;
}

int CSVReader::skippedRows() const {
    return skipped_;
}
