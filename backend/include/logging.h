#pragma once

#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <utility>
#include <vector>

using LogField = std::pair<std::string, std::string>;
using LogFields = std::vector<LogField>;

namespace AppLogger {

void initialize();
void shutdown();

void info(const std::string& eventName, const LogFields& fields = {});
void warn(const std::string& eventName, const LogFields& fields = {});
void error(const std::string& eventName, const LogFields& fields = {});

}  // namespace AppLogger

#endif
