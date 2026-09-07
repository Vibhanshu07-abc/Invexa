#include "../include/gemini_service.h"
#include "../include/config_service.h"
#include "../include/nlohmann/json.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>

using namespace std;
using nlohmann::json;

namespace {
string createTempRequestPath() {
    char buffer[L_tmpnam];
    tmpnam(buffer);
    return string(buffer) + ".json";
}

string shellQuote(const string& value) {
    string escaped;
    for (char ch : value) {
        if (ch == '"') {
            escaped += "\\\"";
        } else {
            escaped += ch;
        }
    }
    return "\"" + escaped + "\"";
}
}

GeminiInsightBundle GeminiService::generateInsights(
    const ItemList& items,
    const AnalysisSnapshot& snapshot,
    const vector<BusinessRecommendation>& recommendations) const {
    GeminiInsightBundle bundle;
    bundle.provider = "google-gemini";
    bundle.enabled = isEnabled() && !apiKey().empty();
    bundle.sourceOfTruth = "Deterministic algorithms remain the source of truth.";

    if (!bundle.enabled) {
        bundle.recommendationExplanations = buildFallbackExplanations(recommendations);
        bundle.executiveSummary = buildFallbackSummary(snapshot);
        bundle.demandForecasts = buildFallbackForecasts(items);
        return bundle;
    }

    const string explanationText = callModel(buildExplanationPrompt(recommendations));
    const string summaryText = callModel(buildExecutiveSummaryPrompt(snapshot, recommendations));
    const string forecastText = callModel(buildDemandForecastPrompt(items));

    bundle.recommendationExplanations = parseExplanationLines(explanationText);
    bundle.executiveSummary.summary = trim(summaryText);
    bundle.executiveSummary.status = bundle.executiveSummary.summary.empty() ? "fallback" : "generated";
    bundle.demandForecasts = parseForecastLines(forecastText);

    if (bundle.recommendationExplanations.empty()) {
        bundle.recommendationExplanations = buildFallbackExplanations(recommendations);
    }
    if (bundle.executiveSummary.summary.empty()) {
        bundle.executiveSummary = buildFallbackSummary(snapshot);
    }
    if (bundle.demandForecasts.empty()) {
        bundle.demandForecasts = buildFallbackForecasts(items);
    }

    return bundle;
}

bool GeminiService::isEnabled() const {
    return ConfigService::instance().data().geminiEnabled;
}

string GeminiService::apiKey() const {
    const string envName = ConfigService::instance().data().geminiApiKeyEnv;
    const char* value = getenv(envName.c_str());
    return value != nullptr ? string(value) : "";
}

string GeminiService::callModel(const string& prompt) const {
    const string key = apiKey();
    if (prompt.empty() || key.empty()) {
        return "";
    }

    const string bodyText = "{\"contents\":[{\"parts\":[{\"text\":" + json(prompt).dump() +
        "}]}],\"generationConfig\":{\"temperature\":0.2,\"maxOutputTokens\":512}}";

    const string requestPath = createTempRequestPath();
    const string responsePath = createTempRequestPath();
    ofstream requestFile(requestPath);
    if (!requestFile.is_open()) {
        return "";
    }
    requestFile << bodyText;
    requestFile.close();

    const auto& config = ConfigService::instance().data();
    const string command =
        "curl -s -X POST " + shellQuote(config.geminiApiEndpoint + "?key=" + key) +
        " -H " + shellQuote("Content-Type: application/json") +
        " --data-binary @" + shellQuote(requestPath) +
        " > " + shellQuote(responsePath);

    const int exitCode = system(command.c_str());
    remove(requestPath.c_str());
    if (exitCode != 0) {
        remove(responsePath.c_str());
        return "";
    }

    ifstream responseFile(responsePath);
    if (!responseFile.is_open()) {
        remove(requestPath.c_str());
        remove(responsePath.c_str());
        return "";
    }

    ostringstream responseBuffer;
    responseBuffer << responseFile.rdbuf();
    const string response = responseBuffer.str();
    remove(responsePath.c_str());

    return extractText(response);
}

string GeminiService::extractText(const string& response) const {
    const regex pattern("\"text\"\\s*:\\s*\"((?:\\\\.|[^\\\\\"])*)\"");
    smatch match;
    if (!regex_search(response, match, pattern) || match.size() < 2) {
        return "";
    }

    string text = match[1].str();
    text = regex_replace(text, regex("\\\\n"), "\n");
    text = regex_replace(text, regex("\\\\\""), "\"");
    text = regex_replace(text, regex("\\\\\\\\"), "\\");
    return trim(text);
}

string GeminiService::buildExplanationPrompt(
    const vector<BusinessRecommendation>& recommendations) const {
    const int limit = min(static_cast<int>(recommendations.size()),
                          ConfigService::instance().data().geminiExplanationLimit);
    if (limit <= 0) {
        return "";
    }

    ostringstream prompt;
    prompt << "You are explaining deterministic warehouse recommendations.\n"
           << "Do not invent new decisions. Algorithms remain source of truth.\n"
           << "Return exactly " << limit << " lines.\n"
           << "Format each line as: category|title|explanation\n";

    for (int i = 0; i < limit; ++i) {
        const auto& recommendation = recommendations[i];
        prompt << recommendation.category << " | "
               << recommendation.title << " | "
               << recommendation.rationale << "\n";
    }

    return prompt.str();
}

string GeminiService::buildExecutiveSummaryPrompt(
    const AnalysisSnapshot& snapshot,
    const vector<BusinessRecommendation>& recommendations) const {
    ostringstream prompt;
    prompt << "Create a concise executive summary for a warehouse intelligence dashboard.\n"
           << "Algorithms remain the source of truth. Do not change numbers.\n"
           << "Mention total items: " << snapshot.summary.totalItems << ".\n"
           << "Mention high-risk items: " << snapshot.summary.highRiskItems << ".\n"
           << "Mention estimated financial loss: " << snapshot.summary.estimatedFinancialLoss << ".\n"
           << "Mention recommendation count: " << recommendations.size() << ".\n"
           << "Keep it under 120 words.";
    return prompt.str();
}

string GeminiService::buildDemandForecastPrompt(const ItemList& items) const {
    vector<Item> ranked = items;
    sort(ranked.begin(), ranked.end(),
        [](const Item& a, const Item& b) {
            if (a.demand == b.demand) {
                return a.riskScore > b.riskScore;
            }
            return a.demand > b.demand;
        });

    const int limit = min(static_cast<int>(ranked.size()),
                          ConfigService::instance().data().geminiForecastItemLimit);
    if (limit <= 0) {
        return "";
    }

    ostringstream prompt;
    prompt << "Forecast near-term demand for warehouse items using the supplied operational context.\n"
           << "Do not override deterministic algorithms. Return exactly " << limit << " lines.\n"
           << "Format each line as: itemId|projectedDemand|confidence|explanation\n";

    for (int i = 0; i < limit; ++i) {
        const auto& item = ranked[i];
        prompt << item.id << " | " << item.name
               << " | demand=" << item.demand
               << " | mismatch=" << item.mismatch
               << " | frequency=" << item.frequency
               << " | risk=" << item.riskScore << "\n";
    }

    return prompt.str();
}

vector<RecommendationExplanation> GeminiService::buildFallbackExplanations(
    const vector<BusinessRecommendation>& recommendations) const {
    vector<RecommendationExplanation> explanations;
    const int limit = min(static_cast<int>(recommendations.size()),
                          ConfigService::instance().data().geminiExplanationLimit);

    for (int i = 0; i < limit; ++i) {
        RecommendationExplanation explanation;
        explanation.category = recommendations[i].category;
        explanation.title = recommendations[i].title;
        explanation.explanation =
            "Gemini is disabled, so this explanation is derived from deterministic rationale: " +
            recommendations[i].rationale;
        explanation.status = "fallback";
        explanations.push_back(explanation);
    }

    return explanations;
}

ExecutiveSummaryInsight GeminiService::buildFallbackSummary(const AnalysisSnapshot& snapshot) const {
    ExecutiveSummaryInsight summary;
    summary.summary =
        "Deterministic analysis found " + to_string(snapshot.summary.highRiskItems) +
        " high-risk items across " + to_string(snapshot.summary.totalItems) +
        " tracked items with estimated loss " + to_string(static_cast<int>(snapshot.summary.estimatedFinancialLoss)) +
        ". Gemini is disabled, so this executive summary is system-generated.";
    summary.status = "fallback";
    return summary;
}

vector<DemandForecastInsight> GeminiService::buildFallbackForecasts(const ItemList& items) const {
    vector<Item> ranked = items;
    sort(ranked.begin(), ranked.end(),
        [](const Item& a, const Item& b) {
            if (a.demand == b.demand) {
                return a.frequency > b.frequency;
            }
            return a.demand > b.demand;
        });

    vector<DemandForecastInsight> forecasts;
    const int limit = min(static_cast<int>(ranked.size()),
                          ConfigService::instance().data().geminiForecastItemLimit);
    for (int i = 0; i < limit; ++i) {
        DemandForecastInsight forecast;
        forecast.itemId = ranked[i].id;
        forecast.itemName = ranked[i].name;
        forecast.currentDemand = ranked[i].demand;
        forecast.projectedDemand = max(ranked[i].demand,
                                       ranked[i].demand + ranked[i].frequency + max(ranked[i].mismatch, 0) / 2);
        forecast.confidence = "baseline";
        forecast.explanation =
            "Gemini is disabled, so the forecast uses existing demand, frequency, and shortage signals.";
        forecast.status = "fallback";
        forecasts.push_back(forecast);
    }

    return forecasts;
}

vector<RecommendationExplanation> GeminiService::parseExplanationLines(const string& text) const {
    vector<RecommendationExplanation> explanations;
    istringstream input(text);
    string line;
    while (getline(input, line)) {
        if (trim(line).empty()) {
            continue;
        }

        vector<string> parts;
        istringstream lineStream(line);
        string part;
        while (getline(lineStream, part, '|')) {
            parts.push_back(trim(part));
        }

        if (parts.size() < 3) {
            continue;
        }

        RecommendationExplanation explanation;
        explanation.category = parts[0];
        explanation.title = parts[1];
        explanation.explanation = parts[2];
        explanation.status = "generated";
        explanations.push_back(explanation);
    }

    return explanations;
}

vector<DemandForecastInsight> GeminiService::parseForecastLines(const string& text) const {
    vector<DemandForecastInsight> forecasts;
    istringstream input(text);
    string line;
    while (getline(input, line)) {
        if (trim(line).empty()) {
            continue;
        }

        vector<string> parts;
        istringstream lineStream(line);
        string part;
        while (getline(lineStream, part, '|')) {
            parts.push_back(trim(part));
        }

        if (parts.size() < 4) {
            continue;
        }

        DemandForecastInsight forecast{};
        try {
            forecast.itemId = stoi(parts[0]);
            forecast.projectedDemand = stoi(parts[1]);
        } catch (...) {
            continue;
        }
        forecast.itemName = "";
        forecast.currentDemand = 0;
        forecast.confidence = parts[2];
        forecast.explanation = parts[3];
        forecast.status = "generated";
        forecasts.push_back(forecast);
    }

    return forecasts;
}

string GeminiService::trim(const string& value) {
    const auto start = find_if_not(value.begin(), value.end(),
        [](unsigned char ch) { return isspace(ch) != 0; });
    const auto end = find_if_not(value.rbegin(), value.rend(),
        [](unsigned char ch) { return isspace(ch) != 0; }).base();

    if (start >= end) {
        return "";
    }

    return string(start, end);
}
