#pragma once

#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include "repository_configuration.h"

#include <string>
#include <vector>

struct AuthUserConfig {
    std::string username;
    std::string passwordHash;
    std::string role;
};

struct ConfigData {
    std::string inventoryCsvPath;
    std::string outputJsonPath;
    std::string repositoryBackend;
    std::string postgresHost;
    int postgresPort;
    std::string postgresDatabase;
    std::string postgresUsername;
    std::string postgresPassword;
    bool geminiEnabled;
    std::string geminiApiEndpoint;
    std::string geminiApiKeyEnv;
    std::string geminiModel;
    int geminiForecastItemLimit;
    int geminiExplanationLimit;
    int topK;
    int defaultBudget;
    int jsonIndent;
    AuthUserConfig adminUser;
    AuthUserConfig warehouseManagerUser;
    AuthUserConfig analystUser;
    AuthUserConfig viewerUser;
    AuthUserConfig employeeUser;
    AuthUserConfig managerUser;
    std::vector<std::string> defaultCategories;
    std::vector<std::string> defaultZones;
    int defaultDemandMinimum;
    int defaultDemandDivisor;
    int alternateZoneModuloBase;
    double restockDemandBufferMultiplier;
    double stockImpactWeight;
    double noExpectedStockImpact;
    double riskLossDivisor;
    double riskLossWeight;
    double riskDemandDivisor;
    double riskDemandWeight;
    double riskHistoryDivisor;
    double riskHistoryWeight;
    double riskMisplacedWeight;
    double positiveVolatilityDivisor;
    double positiveVolatilityWeight;
    double negativeVolatilityDivisor;
    double negativeVolatilityWeight;
    double itemHighRiskScore;
    int itemHighMismatch;
    double itemHighLoss;
    double itemMediumRiskScore;
    int itemMediumMismatch;
    double itemMediumLoss;
    double profitLossWeight;
    double profitRiskWeight;
    double profitDemandWeight;
    double profitMisplacedBonus;
    double profitFrequencyWeight;
    double greedyLossUrgencyDivisor;
    double greedyLossUrgencyWeight;
    double greedyRiskUrgencyWeight;
    double greedyDemandUrgencyDivisor;
    double greedyDemandUrgencyWeight;
    double greedyFrequencyUrgencyDivisor;
    double greedyFrequencyUrgencyWeight;
    double greedyMisplacedUrgencyWeight;
    double hashHighRiskScore;
    int hashHighMismatch;
    double hashMediumRiskScore;
    int hashMediumMismatch;
    int hashFrequencySevereDelta;
    int hashFrequencyHighDelta;
    int hashFrequencyMediumDelta;
    int hashFrequencySevereValue;
    int hashFrequencyHighValue;
    int hashFrequencyMediumValue;
    int hashFrequencyLowValue;
    double heapCriticalScore;
    double heapWarningScore;
    double heapWatchScore;
    double dpImmediateRiskScore;
    double dpImmediateLoss;
    double dpHighRiskScore;
    double dpHighLoss;
    int dpPreviewRows;
    int dpPreviewCols;
    double graphSimilarRiskThreshold;
    double graphSameCategoryWeight;
    double graphSameLocationWeight;
    double graphRiskWeightBase;
    double graphRiskDifferenceCap;
    double graphRiskDifferenceDivisor;
    double graphSeverityHighRisk;
    double graphSeverityMediumRisk;
    double graphTheftClusterAvgRisk;
    int graphMinSharedLocationCount;
    int graphHighRiskClusterDivisor;
    int demoAuditOneHoursBack;
    int demoAuditTwoHoursBack;
    int demoAuditLossModulo;
    int demoAuditLossUnits;
    int demoSecondAuditLossModulo;
    int demoSecondAuditLossMinimum;
    int demoSecondAuditLossDivisor;
    int demoTransitModulo;
    std::string demoTransitLocation;
    int summaryRecommendationLimit;
};

class ConfigService {
public:
    static const ConfigService& instance();

    const ConfigData& data() const;
    RepositoryConfiguration repositoryConfiguration() const;

private:
    ConfigService();

    ConfigData data_;

    static std::string loadFile(const std::string& path);
    static std::string findString(const std::string& source, const std::string& key, const std::string& fallback);
    static int findInt(const std::string& source, const std::string& key, int fallback);
    static double findDouble(const std::string& source, const std::string& key, double fallback);
    static bool findBool(const std::string& source, const std::string& key, bool fallback);
    static std::vector<std::string> findStringArray(const std::string& source,
                                                    const std::string& key,
                                                    const std::vector<std::string>& fallback);
    static RepositoryBackend parseRepositoryBackend(const std::string& backend);
};

#endif
