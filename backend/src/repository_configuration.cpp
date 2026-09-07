#include "../include/repository_configuration.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

using namespace std;

RepositoryConfigurationLoader::RepositoryConfigurationLoader(string configPath, string defaultCsvPath)
    : configPath_(std::move(configPath)), defaultCsvPath_(std::move(defaultCsvPath)) {}

string RepositoryConfigurationLoader::trim(const string& value) {
    const auto begin = find_if_not(value.begin(), value.end(),
        [](unsigned char ch) { return std::isspace(ch) != 0; });
    if (begin == value.end()) {
        return "";
    }

    const auto end = find_if_not(value.rbegin(), value.rend(),
        [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    return string(begin, end);
}

RepositoryBackend RepositoryConfigurationLoader::parseBackend(const string& value) {
    string normalized = value;
    transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (normalized == "postgres" || normalized == "postgresql") {
        return RepositoryBackend::PostgreSQL;
    }

    return RepositoryBackend::CSV;
}

RepositoryConfiguration RepositoryConfigurationLoader::load() const {
    RepositoryConfiguration configuration;
    configuration.csvPath = defaultCsvPath_;

    ifstream file(configPath_);
    if (!file.is_open()) {
        return configuration;
    }

    string line;
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const size_t separator = line.find('=');
        if (separator == string::npos) {
            continue;
        }

        const string key = trim(line.substr(0, separator));
        const string value = trim(line.substr(separator + 1));

        if (key == "backend") {
            configuration.backend = parseBackend(value);
        } else if (key == "csv_path" && !value.empty()) {
            configuration.csvPath = value;
        } else if (key == "postgres_host" && !value.empty()) {
            configuration.postgresql.host = value;
        } else if (key == "postgres_port" && !value.empty()) {
            configuration.postgresql.port = stoi(value);
        } else if (key == "postgres_database" && !value.empty()) {
            configuration.postgresql.database = value;
        } else if (key == "postgres_username" && !value.empty()) {
            configuration.postgresql.username = value;
        } else if (key == "postgres_password" && !value.empty()) {
            configuration.postgresql.password = value;
        }
    }

    return configuration;
}
