#pragma once

#ifndef REPOSITORY_CONFIGURATION_H
#define REPOSITORY_CONFIGURATION_H

#include <string>

enum class RepositoryBackend {
    CSV,
    PostgreSQL
};

struct PostgreSQLConfiguration {
    std::string host;
    int port = 0;
    std::string database;
    std::string username;
    std::string password;
};

struct RepositoryConfiguration {
    RepositoryBackend backend = RepositoryBackend::CSV;
    std::string csvPath;
    PostgreSQLConfiguration postgresql;
};

class RepositoryConfigurationLoader {
public:
    RepositoryConfigurationLoader(std::string configPath, std::string defaultCsvPath);

    RepositoryConfiguration load() const;

private:
    std::string configPath_;
    std::string defaultCsvPath_;

    static std::string trim(const std::string& value);
    static RepositoryBackend parseBackend(const std::string& value);
};

#endif
