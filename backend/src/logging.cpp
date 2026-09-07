#include "../include/logging.h"

#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if __has_include(<spdlog/sinks/basic_file_sink.h>) && __has_include(<spdlog/sinks/stdout_color_sinks.h>) && __has_include(<spdlog/spdlog.h>)
#define SMARTINVENTORY_HAS_SPDLOG 1
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#endif

using namespace std;

namespace {

string nowUtcIso8601() {
    const time_t now = time(nullptr);
    tm timeInfo{};
#if defined(_WIN32) && !defined(__MINGW32__)
    gmtime_s(&timeInfo, &now);
#elif defined(__unix__) || defined(__APPLE__)
    gmtime_r(&now, &timeInfo);
#else
    const tm* utcTime = gmtime(&now);
    if (utcTime != nullptr) {
        timeInfo = *utcTime;
    }
#endif

    ostringstream output;
    output << put_time(&timeInfo, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

string envOrDefault(const char* name, const string& fallback) {
    const char* value = getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return value;
}

string escapeJson(const string& value) {
    ostringstream escaped;
    for (char ch : value) {
        switch (ch) {
            case '\\': escaped << "\\\\"; break;
            case '"': escaped << "\\\""; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default: escaped << ch; break;
        }
    }
    return escaped.str();
}

string buildPayload(const string& level, const string& eventName, const LogFields& fields) {
    ostringstream payload;
    payload << "{"
            << "\"timestamp\":\"" << escapeJson(nowUtcIso8601()) << "\","
            << "\"level\":\"" << escapeJson(level) << "\","
            << "\"event\":\"" << escapeJson(eventName) << "\","
            << "\"service\":\"smartinventory-backend\"";

    for (const auto& field : fields) {
        payload << ",\"" << escapeJson(field.first) << "\":\"" << escapeJson(field.second) << "\"";
    }

    payload << "}";
    return payload.str();
}

#ifdef SMARTINVENTORY_HAS_SPDLOG
shared_ptr<spdlog::logger> buildLogger() {
    vector<spdlog::sink_ptr> sinks;
    sinks.push_back(make_shared<spdlog::sinks::stdout_color_sink_mt>());

    const string logFilePath = envOrDefault("SMARTINVENTORY_LOG_FILE_PATH", "");
    if (!logFilePath.empty()) {
        sinks.push_back(make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true));
    }

    auto logger = make_shared<spdlog::logger>("smartinventory", sinks.begin(), sinks.end());
    logger->set_pattern("%v");

    const string level = envOrDefault("SMARTINVENTORY_LOG_LEVEL", "info");
    if (level == "debug") logger->set_level(spdlog::level::debug);
    else if (level == "warn") logger->set_level(spdlog::level::warn);
    else if (level == "error") logger->set_level(spdlog::level::err);
    else logger->set_level(spdlog::level::info);

    logger->flush_on(spdlog::level::info);
    return logger;
}

shared_ptr<spdlog::logger> loggerInstance;
#endif

void writeLog(const string& level, const string& eventName, const LogFields& fields) {
    const string line = buildPayload(level, eventName, fields);
#ifdef SMARTINVENTORY_HAS_SPDLOG
    if (!loggerInstance) {
        loggerInstance = buildLogger();
    }

    if (level == "error") loggerInstance->error(line);
    else if (level == "warn") loggerInstance->warn(line);
    else loggerInstance->info(line);
#else
    cout << line << '\n';
    cout.flush();
#endif
}

}  // namespace

namespace AppLogger {

void initialize() {
#ifdef SMARTINVENTORY_HAS_SPDLOG
    if (!loggerInstance) {
        loggerInstance = buildLogger();
    }
#endif
}

void shutdown() {
#ifdef SMARTINVENTORY_HAS_SPDLOG
    loggerInstance.reset();
    spdlog::shutdown();
#endif
}

void info(const string& eventName, const LogFields& fields) {
    writeLog("info", eventName, fields);
}

void warn(const string& eventName, const LogFields& fields) {
    writeLog("warn", eventName, fields);
}

void error(const string& eventName, const LogFields& fields) {
    writeLog("error", eventName, fields);
}

}  // namespace AppLogger
