#include "../include/config_service.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>

using namespace std;

namespace {
const char* CONFIG_JSON_PATH = "./config/config.json";

string envOrDefault(const char* key, const string& fallback) {
    const char* value = getenv(key);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return value;
}

int envOrDefaultInt(const char* key, int fallback) {
    const char* value = getenv(key);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    try {
        return stoi(value);
    } catch (...) {
        return fallback;
    }
}

bool envOrDefaultBool(const char* key, bool fallback) {
    const char* value = getenv(key);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    string normalized = value;
    for (char& ch : normalized) {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        return false;
    }
    return fallback;
}
}

const ConfigService& ConfigService::instance() {
    static ConfigService service;
    return service;
}

const ConfigData& ConfigService::data() const {
    return data_;
}

ConfigService::ConfigService() {
    const string configPath = envOrDefault("SMARTINVENTORY_CONFIG_PATH", CONFIG_JSON_PATH);
    const string source = loadFile(configPath);

    data_.inventoryCsvPath = envOrDefault(
        "SMARTINVENTORY_INVENTORY_CSV_PATH",
        findString(source, "inventoryCsvPath", "../data/inventory.csv"));
    data_.outputJsonPath = envOrDefault(
        "SMARTINVENTORY_OUTPUT_JSON_PATH",
        findString(source, "outputJsonPath", "./output/result.json"));
    data_.repositoryBackend = envOrDefault(
        "SMARTINVENTORY_REPOSITORY_BACKEND",
        findString(source, "repositoryBackend", "csv"));
    data_.postgresHost = envOrDefault(
        "SMARTINVENTORY_POSTGRES_HOST",
        findString(source, "postgresHost", "localhost"));
    data_.postgresPort = envOrDefaultInt(
        "SMARTINVENTORY_POSTGRES_PORT",
        findInt(source, "postgresPort", 5432));
    data_.postgresDatabase = envOrDefault(
        "SMARTINVENTORY_POSTGRES_DATABASE",
        findString(source, "postgresDatabase", "smart_inventory"));
    data_.postgresUsername = envOrDefault(
        "SMARTINVENTORY_POSTGRES_USERNAME",
        findString(source, "postgresUsername", "postgres"));
    data_.postgresPassword = envOrDefault(
        "SMARTINVENTORY_POSTGRES_PASSWORD",
        findString(source, "postgresPassword", "postgres"));
    data_.geminiEnabled = envOrDefaultBool(
        "SMARTINVENTORY_GEMINI_ENABLED",
        findBool(source, "geminiEnabled", false));
    data_.geminiApiEndpoint = envOrDefault(
        "SMARTINVENTORY_GEMINI_API_ENDPOINT",
        findString(source,
                   "geminiApiEndpoint",
                   "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent"));
    data_.geminiApiKeyEnv = envOrDefault(
        "SMARTINVENTORY_GEMINI_API_KEY_ENV",
        findString(source, "geminiApiKeyEnv", "GEMINI_API_KEY"));
    data_.geminiModel = envOrDefault(
        "SMARTINVENTORY_GEMINI_MODEL",
        findString(source, "geminiModel", "gemini-2.5-flash"));
    data_.geminiForecastItemLimit = envOrDefaultInt(
        "SMARTINVENTORY_GEMINI_FORECAST_ITEM_LIMIT",
        findInt(source, "geminiForecastItemLimit", 5));
    data_.geminiExplanationLimit = envOrDefaultInt(
        "SMARTINVENTORY_GEMINI_EXPLANATION_LIMIT",
        findInt(source, "geminiExplanationLimit", 3));
    data_.topK = envOrDefaultInt("SMARTINVENTORY_TOP_K", findInt(source, "topK", 5));
    data_.defaultBudget = envOrDefaultInt(
        "SMARTINVENTORY_DEFAULT_BUDGET",
        findInt(source, "defaultBudget", 2500));
    data_.jsonIndent = envOrDefaultInt(
        "SMARTINVENTORY_JSON_INDENT",
        findInt(source, "jsonIndent", 2));

    data_.adminUser.username = envOrDefault(
        "SMARTINVENTORY_ADMIN_USERNAME",
        findString(source, "adminUsername", "admin"));
    data_.adminUser.passwordHash = envOrDefault(
        "SMARTINVENTORY_ADMIN_PASSWORD_HASH",
        findString(source, "adminPasswordHash", ""));
    data_.adminUser.role = envOrDefault(
        "SMARTINVENTORY_ADMIN_ROLE",
        findString(source, "adminRole", "Admin"));

    data_.warehouseManagerUser.username = envOrDefault(
        "SMARTINVENTORY_WAREHOUSE_MANAGER_USERNAME",
        findString(source, "warehouseManagerUsername", "warehouse_manager"));
    data_.warehouseManagerUser.passwordHash = envOrDefault(
        "SMARTINVENTORY_WAREHOUSE_MANAGER_PASSWORD_HASH",
        findString(source, "warehouseManagerPasswordHash", ""));
    data_.warehouseManagerUser.role = envOrDefault(
        "SMARTINVENTORY_WAREHOUSE_MANAGER_ROLE",
        findString(source, "warehouseManagerRole", "Warehouse Manager"));

    data_.analystUser.username = envOrDefault(
        "SMARTINVENTORY_ANALYST_USERNAME",
        findString(source, "analystUsername", "analyst"));
    data_.analystUser.passwordHash = envOrDefault(
        "SMARTINVENTORY_ANALYST_PASSWORD_HASH",
        findString(source, "analystPasswordHash", ""));
    data_.analystUser.role = envOrDefault(
        "SMARTINVENTORY_ANALYST_ROLE",
        findString(source, "analystRole", "Analyst"));

    data_.viewerUser.username = envOrDefault(
        "SMARTINVENTORY_VIEWER_USERNAME",
        findString(source, "viewerUsername", "viewer"));
    data_.viewerUser.passwordHash = envOrDefault(
        "SMARTINVENTORY_VIEWER_PASSWORD_HASH",
        findString(source, "viewerPasswordHash", ""));
    data_.viewerUser.role = envOrDefault(
        "SMARTINVENTORY_VIEWER_ROLE",
        findString(source, "viewerRole", "Viewer"));

    data_.employeeUser.username = envOrDefault(
        "SMARTINVENTORY_EMPLOYEE_USERNAME",
        findString(source, "employeeUsername", data_.warehouseManagerUser.username));
    data_.employeeUser.passwordHash = envOrDefault(
        "SMARTINVENTORY_EMPLOYEE_PASSWORD_HASH",
        findString(source, "employeePasswordHash", data_.warehouseManagerUser.passwordHash));
    data_.employeeUser.role = envOrDefault(
        "SMARTINVENTORY_EMPLOYEE_ROLE",
        findString(source, "employeeRole", data_.warehouseManagerUser.role));

    data_.managerUser.username = envOrDefault(
        "SMARTINVENTORY_MANAGER_USERNAME",
        findString(source, "managerUsername", data_.adminUser.username));
    data_.managerUser.passwordHash = envOrDefault(
        "SMARTINVENTORY_MANAGER_PASSWORD_HASH",
        findString(source, "managerPasswordHash", data_.adminUser.passwordHash));
    data_.managerUser.role = envOrDefault(
        "SMARTINVENTORY_MANAGER_ROLE",
        findString(source, "managerRole", data_.adminUser.role));

    data_.defaultCategories = findStringArray(source, "defaultCategories",
        {"Electronics", "Groceries", "Hardware", "Apparel", "Medical"});
    data_.defaultZones = findStringArray(source, "defaultZones",
        {"Zone-A", "Zone-B", "Zone-C", "Zone-D", "Zone-E"});
    data_.defaultDemandMinimum = findInt(source, "defaultDemandMinimum", 5);
    data_.defaultDemandDivisor = findInt(source, "defaultDemandDivisor", 5);
    data_.alternateZoneModuloBase = findInt(source, "alternateZoneModuloBase", 5);
    data_.restockDemandBufferMultiplier = findDouble(source, "restockDemandBufferMultiplier", 1.5);
    data_.stockImpactWeight = findDouble(source, "stockImpactWeight", 40.0);
    data_.noExpectedStockImpact = findDouble(source, "noExpectedStockImpact", 25.0);
    data_.riskLossDivisor = findDouble(source, "riskLossDivisor", 1500.0);
    data_.riskLossWeight = findDouble(source, "riskLossWeight", 25.0);
    data_.riskDemandDivisor = findDouble(source, "riskDemandDivisor", 120.0);
    data_.riskDemandWeight = findDouble(source, "riskDemandWeight", 15.0);
    data_.riskHistoryDivisor = findDouble(source, "riskHistoryDivisor", 6.0);
    data_.riskHistoryWeight = findDouble(source, "riskHistoryWeight", 10.0);
    data_.riskMisplacedWeight = findDouble(source, "riskMisplacedWeight", 10.0);
    data_.positiveVolatilityDivisor = findDouble(source, "positiveVolatilityDivisor", 40.0);
    data_.positiveVolatilityWeight = findDouble(source, "positiveVolatilityWeight", 10.0);
    data_.negativeVolatilityDivisor = findDouble(source, "negativeVolatilityDivisor", 40.0);
    data_.negativeVolatilityWeight = findDouble(source, "negativeVolatilityWeight", 4.0);
    data_.itemHighRiskScore = findDouble(source, "itemHighRiskScore", 70.0);
    data_.itemHighMismatch = findInt(source, "itemHighMismatch", 20);
    data_.itemHighLoss = findDouble(source, "itemHighLoss", 1200.0);
    data_.itemMediumRiskScore = findDouble(source, "itemMediumRiskScore", 40.0);
    data_.itemMediumMismatch = findInt(source, "itemMediumMismatch", 8);
    data_.itemMediumLoss = findDouble(source, "itemMediumLoss", 250.0);
    data_.profitLossWeight = findDouble(source, "profitLossWeight", 0.45);
    data_.profitRiskWeight = findDouble(source, "profitRiskWeight", 1.20);
    data_.profitDemandWeight = findDouble(source, "profitDemandWeight", 2.0);
    data_.profitMisplacedBonus = findDouble(source, "profitMisplacedBonus", 20.0);
    data_.profitFrequencyWeight = findDouble(source, "profitFrequencyWeight", 12.0);
    data_.greedyLossUrgencyDivisor = findDouble(source, "greedyLossUrgencyDivisor", 1500.0);
    data_.greedyLossUrgencyWeight = findDouble(source, "greedyLossUrgencyWeight", 35.0);
    data_.greedyRiskUrgencyWeight = findDouble(source, "greedyRiskUrgencyWeight", 35.0);
    data_.greedyDemandUrgencyDivisor = findDouble(source, "greedyDemandUrgencyDivisor", 120.0);
    data_.greedyDemandUrgencyWeight = findDouble(source, "greedyDemandUrgencyWeight", 15.0);
    data_.greedyFrequencyUrgencyDivisor = findDouble(source, "greedyFrequencyUrgencyDivisor", 6.0);
    data_.greedyFrequencyUrgencyWeight = findDouble(source, "greedyFrequencyUrgencyWeight", 10.0);
    data_.greedyMisplacedUrgencyWeight = findDouble(source, "greedyMisplacedUrgencyWeight", 5.0);
    data_.hashHighRiskScore = findDouble(source, "hashHighRiskScore", 70.0);
    data_.hashHighMismatch = findInt(source, "hashHighMismatch", 20);
    data_.hashMediumRiskScore = findDouble(source, "hashMediumRiskScore", 40.0);
    data_.hashMediumMismatch = findInt(source, "hashMediumMismatch", 8);
    data_.hashFrequencySevereDelta = findInt(source, "hashFrequencySevereDelta", 25);
    data_.hashFrequencyHighDelta = findInt(source, "hashFrequencyHighDelta", 15);
    data_.hashFrequencyMediumDelta = findInt(source, "hashFrequencyMediumDelta", 8);
    data_.hashFrequencySevereValue = findInt(source, "hashFrequencySevereValue", 5);
    data_.hashFrequencyHighValue = findInt(source, "hashFrequencyHighValue", 4);
    data_.hashFrequencyMediumValue = findInt(source, "hashFrequencyMediumValue", 3);
    data_.hashFrequencyLowValue = findInt(source, "hashFrequencyLowValue", 2);
    data_.heapCriticalScore = findDouble(source, "heapCriticalScore", 80.0);
    data_.heapWarningScore = findDouble(source, "heapWarningScore", 60.0);
    data_.heapWatchScore = findDouble(source, "heapWatchScore", 40.0);
    data_.dpImmediateRiskScore = findDouble(source, "dpImmediateRiskScore", 80.0);
    data_.dpImmediateLoss = findDouble(source, "dpImmediateLoss", 1500.0);
    data_.dpHighRiskScore = findDouble(source, "dpHighRiskScore", 60.0);
    data_.dpHighLoss = findDouble(source, "dpHighLoss", 600.0);
    data_.dpPreviewRows = findInt(source, "dpPreviewRows", 6);
    data_.dpPreviewCols = findInt(source, "dpPreviewCols", 12);
    data_.graphSimilarRiskThreshold = findDouble(source, "graphSimilarRiskThreshold", 12.0);
    data_.graphSameCategoryWeight = findDouble(source, "graphSameCategoryWeight", 0.40);
    data_.graphSameLocationWeight = findDouble(source, "graphSameLocationWeight", 0.35);
    data_.graphRiskWeightBase = findDouble(source, "graphRiskWeightBase", 0.25);
    data_.graphRiskDifferenceCap = findDouble(source, "graphRiskDifferenceCap", 25.0);
    data_.graphRiskDifferenceDivisor = findDouble(source, "graphRiskDifferenceDivisor", 100.0);
    data_.graphSeverityHighRisk = findDouble(source, "graphSeverityHighRisk", 70.0);
    data_.graphSeverityMediumRisk = findDouble(source, "graphSeverityMediumRisk", 45.0);
    data_.graphTheftClusterAvgRisk = findDouble(source, "graphTheftClusterAvgRisk", 65.0);
    data_.graphMinSharedLocationCount = findInt(source, "graphMinSharedLocationCount", 2);
    data_.graphHighRiskClusterDivisor = findInt(source, "graphHighRiskClusterDivisor", 2);
    data_.demoAuditOneHoursBack = findInt(source, "demoAuditOneHoursBack", 6);
    data_.demoAuditTwoHoursBack = findInt(source, "demoAuditTwoHoursBack", 3);
    data_.demoAuditLossModulo = findInt(source, "demoAuditLossModulo", 6);
    data_.demoAuditLossUnits = findInt(source, "demoAuditLossUnits", 3);
    data_.demoSecondAuditLossModulo = findInt(source, "demoSecondAuditLossModulo", 3);
    data_.demoSecondAuditLossMinimum = findInt(source, "demoSecondAuditLossMinimum", 4);
    data_.demoSecondAuditLossDivisor = findInt(source, "demoSecondAuditLossDivisor", 6);
    data_.demoTransitModulo = findInt(source, "demoTransitModulo", 5);
    data_.demoTransitLocation = findString(source, "demoTransitLocation", "Transit-Bay");
    data_.summaryRecommendationLimit = findInt(source, "summaryRecommendationLimit", 5);
}

RepositoryConfiguration ConfigService::repositoryConfiguration() const {
    RepositoryConfiguration configuration;
    configuration.backend = parseRepositoryBackend(data_.repositoryBackend);
    configuration.csvPath = data_.inventoryCsvPath;
    configuration.postgresql.host = data_.postgresHost;
    configuration.postgresql.port = data_.postgresPort;
    configuration.postgresql.database = data_.postgresDatabase;
    configuration.postgresql.username = data_.postgresUsername;
    configuration.postgresql.password = data_.postgresPassword;
    return configuration;
}

string ConfigService::loadFile(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        return "";
    }

    ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

string ConfigService::findString(const string& source, const string& key, const string& fallback) {
    const regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    smatch match;
    if (regex_search(source, match, pattern) && match.size() > 1) {
        return match[1].str();
    }
    return fallback;
}

int ConfigService::findInt(const string& source, const string& key, int fallback) {
    const regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    smatch match;
    if (regex_search(source, match, pattern) && match.size() > 1) {
        return stoi(match[1].str());
    }
    return fallback;
}

double ConfigService::findDouble(const string& source, const string& key, double fallback) {
    const regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+(?:\\.\\d+)?)");
    smatch match;
    if (regex_search(source, match, pattern) && match.size() > 1) {
        return stod(match[1].str());
    }
    return fallback;
}

bool ConfigService::findBool(const string& source, const string& key, bool fallback) {
    const regex pattern("\"" + key + "\"\\s*:\\s*(true|false)");
    smatch match;
    if (regex_search(source, match, pattern) && match.size() > 1) {
        return match[1].str() == "true";
    }
    return fallback;
}

vector<string> ConfigService::findStringArray(const string& source,
                                              const string& key,
                                              const vector<string>& fallback) {
    const regex arrayPattern("\"" + key + "\"\\s*:\\s*\\[([^\\]]*)\\]");
    smatch arrayMatch;
    if (!(regex_search(source, arrayMatch, arrayPattern) && arrayMatch.size() > 1)) {
        return fallback;
    }

    const string arrayContent = arrayMatch[1].str();
    const regex itemPattern("\"([^\"]*)\"");
    vector<string> result;
    for (sregex_iterator it(arrayContent.begin(), arrayContent.end(), itemPattern), end; it != end; ++it) {
        result.push_back((*it)[1].str());
    }

    return result.empty() ? fallback : result;
}

RepositoryBackend ConfigService::parseRepositoryBackend(const string& backend) {
    string normalized = backend;
    for (char& ch : normalized) {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    return (normalized == "postgres" || normalized == "postgresql")
        ? RepositoryBackend::PostgreSQL
        : RepositoryBackend::CSV;
}
