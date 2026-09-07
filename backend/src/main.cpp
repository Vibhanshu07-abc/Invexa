#include "../include/csv_reader.h"
#include "../include/csv_repository.h"
#include "../include/config_service.h"
#include "../include/hash_algo.h"
#include "../include/heap_algo.h"
#include "../include/greedy_algo.h"
#include "../include/dp_algo.h"
#include "../include/graph_algo.h"
#include "../include/inventory_analysis_service.h"
#include "../include/audit_history.h"
#include "../include/dashboard_serializer.h"
#include "../include/login.h"
#include "../include/logging.h"
#include "../include/monitoring_registry.h"
#include "../include/optimization_engine.h"
#include "../include/repository_configuration.h"
#include "../include/repository_factory.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <limits>
#include <cerrno>
#include <memory>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <thread>
#include <sys/stat.h>
#include <sys/types.h>
#endif

using namespace std;

namespace {

string envOrDefault(const char* key, const string& fallback) {
    const char* value = getenv(key);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return value;
}

bool envOrDefaultBool(const char* key, bool fallback) {
    const char* value = getenv(key);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    string normalized = value;
    transform(normalized.begin(), normalized.end(), normalized.begin(),
              [](unsigned char ch) { return static_cast<char>(tolower(ch)); });

    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        return false;
    }
    return fallback;
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

struct RuntimeOptions {
    bool serviceMode;
    string startupMode;
    int refreshIntervalSeconds;
    bool runOnce;
};

RuntimeOptions loadRuntimeOptions() {
    return {
        envOrDefaultBool("SMARTINVENTORY_SERVICE_MODE", false),
        envOrDefault("SMARTINVENTORY_STARTUP_MODE", "demo"),
        max(0, envOrDefaultInt("SMARTINVENTORY_SERVICE_REFRESH_SECONDS", 30)),
        envOrDefaultBool("SMARTINVENTORY_RUN_ONCE", false)
    };
}

}  // namespace

static string toStr(double v, int prec = 2) {
    ostringstream oss;
    oss << fixed << setprecision(prec) << v;
    return oss.str();
}

static string timestampNow() {
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", t);
    return string(buf);
}

static string timestampOffset(int hoursBack) {
    time_t now = time(nullptr) - hoursBack * 3600;
    tm* t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", t);
    return string(buf);
}

static void logLine(const string& msg) {
    AppLogger::info("application_log", {{"message", msg}});
    cout << "[MAIN] " << msg << "\n";
}

static void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

static string promptString(const string& label) {
    cout << label;
    string value;
    getline(cin, value);
    return value;
}

static int promptInt(const string& label, int minValue = numeric_limits<int>::min()) {
    while (true) {
        cout << label;
        int value;
        if (cin >> value) {
            clearInput();
            if (value >= minValue) return value;
        } else {
            clearInput();
        }
        cout << "Invalid input. Please try again.\n";
    }
}

static double promptDouble(const string& label, double minValue = 0.0) {
    while (true) {
        cout << label;
        double value;
        if (cin >> value) {
            clearInput();
            if (value >= minValue) return value;
        } else {
            clearInput();
        }
        cout << "Invalid input. Please try again.\n";
    }
}


static bool writeTextFile(const string& path, const string& contents) {
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos != string::npos) {
        string parent = path.substr(0, slashPos);
        if (!parent.empty()) {
#ifdef _WIN32
            int result = _mkdir(parent.c_str());
#else
            int result = mkdir(parent.c_str(), 0755);
#endif
            if (result != 0 && errno != EEXIST) {
                logLine("ERROR: Cannot create directory for " + path);
                return false;
            }
        }
    }

    ofstream out(path);
    if (!out.is_open()) {
        logLine("ERROR: Cannot write file " + path);
        return false;
    }

    out << contents;
    return true;
}

static string monitoringOutputPath() {
    const string configuredPath = envOrDefault("SMARTINVENTORY_MONITORING_PATH", "");
    if (!configuredPath.empty()) {
        return configuredPath;
    }

    const string outputPath = ConfigService::instance().data().outputJsonPath;
    const size_t slashPos = outputPath.find_last_of("/\\");
    if (slashPos == string::npos) {
        return "monitoring.json";
    }

    return outputPath.substr(0, slashPos + 1) + "monitoring.json";
}

static void writeJSON(const string& mode,
                      const ItemList& items,
                      const AuditHistory& history,
                      const AnalysisSnapshot& snapshot,
                      int budget) {
    MonitoringRegistry::recordInventorySnapshot(items);
    DashboardSerializer serializer;
    const string payload =
        serializer.serialize(timestampNow(), mode, budget, items, history, snapshot)
            .dump(ConfigService::instance().data().jsonIndent) + "\n";
    const string outputPath = ConfigService::instance().data().outputJsonPath;
    if (writeTextFile(outputPath, payload)) {
        logLine("JSON written to " + outputPath);
    }

    const string monitoringPayload = MonitoringRegistry::snapshot().dump(
        ConfigService::instance().data().jsonIndent) + "\n";
    const string metricsPath = monitoringOutputPath();
    if (writeTextFile(metricsPath, monitoringPayload)) {
        logLine("Monitoring written to " + metricsPath);
    }
}

static Item buildManualItem(int id) {
    string name = promptString("Item name: ");
    string category = promptString("Category: ");
    int expected = promptInt("Expected stock: ", 0);
    int actual = promptInt("Actual stock: ", 0);
    double price = promptDouble("Unit price: ", 0.0);
    int demand = promptInt("Demand score: ", 0);
    string expectedLocation = promptString("Expected location: ");
    string currentLocation = promptString("Current location: ");
    return Item(id, name, category, expected, actual, price, demand, expectedLocation, currentLocation);
}

static bool hasItemId(const ItemList& items, int id) {
    for (const auto& item : items) {
        if (item.id == id) return true;
    }
    return false;
}

static Item* findItem(ItemList& items, int id) {
    for (auto& item : items) {
        if (item.id == id) return &item;
    }
    return nullptr;
}

static void seedDemoAudits(const ItemList& currentItems, AuditHistory& history) {
    const auto& config = ConfigService::instance().data();
    ItemList auditOne = currentItems;
    ItemList auditTwo = currentItems;
    ItemList auditThree = currentItems;

    for (auto& item : auditOne) {
        item.actual = item.expected;
        item.currentLocation = item.expectedLocation;
        if (item.id % config.demoAuditLossModulo == 0) {
            item.actual = max(0, item.expected - config.demoAuditLossUnits);
        }
        item.refreshDerived();
    }

    for (auto& item : auditTwo) {
        if (item.id % config.demoSecondAuditLossModulo == 0) {
            item.actual = max(0, item.expected - max(config.demoSecondAuditLossMinimum,
                                                      item.expected / config.demoSecondAuditLossDivisor));
        }
        if (item.id % config.demoTransitModulo == 0) item.currentLocation = config.demoTransitLocation;
        item.refreshDerived();
    }

    for (auto& item : auditThree) {
        item.refreshDerived();
    }

    history.addAudit(timestampOffset(config.demoAuditOneHoursBack), auditOne);
    history.addAudit(timestampOffset(config.demoAuditTwoHoursBack), auditTwo);
    history.addAudit(timestampNow(), auditThree);
}

static ItemList loadDatasetForMode(string& modeLabel, InventoryRepository& inventoryRepository) {
    while (true) {
        cout << "Select Mode\n";
        cout << "1. Demo Mode\n";
        cout << "2. Real-Time Mode\n";
        cout << "Choice: ";
        int choice;
        if (!(cin >> choice)) {
            clearInput();
            continue;
        }
        clearInput();

        if (choice == 1) {
            modeLabel = "Demo";
            return inventoryRepository.loadInventory();
        }

        if (choice == 2) {
            modeLabel = "Real-Time";
            ItemList items;
            int count = promptInt("How many items do you want to enter initially? ", 0);
            for (int i = 0; i < count; ++i) {
                int id = promptInt("Item id: ", 1);
                while (hasItemId(items, id)) {
                    cout << "ID already exists. Enter a new id.\n";
                    id = promptInt("Item id: ", 1);
                }
                items.push_back(buildManualItem(id));
            }
            return items;
        }
    }
}

static ItemList loadDatasetForModeNonInteractive(const string& startupMode,
                                                 string& modeLabel,
                                                 InventoryRepository& inventoryRepository) {
    string normalized = startupMode;
    transform(normalized.begin(), normalized.end(), normalized.begin(),
              [](unsigned char ch) { return static_cast<char>(tolower(ch)); });

    modeLabel = (normalized == "realtime" || normalized == "real-time") ? "Real-Time" : "Demo";
    return inventoryRepository.loadInventory();
}

static void performAudit(ItemList& items, AuditHistory& history, AuditRepository& auditRepository) {
    if (items.empty()) {
        cout << "No items available for audit.\n";
        return;
    }

    int updates = promptInt("How many items will you verify in this audit? ", 0);
    for (int i = 0; i < updates; ++i) {
        int id = promptInt("Audit item id: ", 1);
        Item* item = findItem(items, id);
        if (!item) {
            cout << "Item not found.\n";
            continue;
        }

        item->actual = promptInt("Updated actual stock: ", 0);
        item->currentLocation = promptString("Updated current location: ");
        item->refreshDerived();
    }

    history.addAudit(timestampNow(), items);
    const auto& audits = history.getAudits();
    if (!audits.empty()) {
        auditRepository.saveAudit(audits.back());
    }
    cout << "Audit recorded successfully.\n";
}

static void addItemInteractive(ItemList& items) {
    int id = promptInt("New item id: ", 1);
    while (hasItemId(items, id)) {
        cout << "ID already exists. Enter a new id.\n";
        id = promptInt("New item id: ", 1);
    }

    items.push_back(buildManualItem(id));
    cout << "Item added successfully.\n";
}

static void updateStockInteractive(ItemList& items) {
    if (items.empty()) {
        cout << "No items available to update.\n";
        return;
    }

    int id = promptInt("Item id to update: ", 1);
    Item* item = findItem(items, id);
    if (!item) {
        cout << "Item not found.\n";
        return;
    }

    item->actual = promptInt("New actual stock: ", 0);
    item->currentLocation = promptString("Current location: ");
    item->demand = promptInt("Demand score: ", 0);
    item->refreshDerived();
    cout << "Stock updated successfully.\n";
}

static void printMisplacedItems(const vector<Item>& misplaced) {
    cout << "\n====== MISPLACED ITEMS ======\n";
    if (misplaced.empty()) {
        cout << "All items are in the correct location.\n";
        cout << "=============================\n\n";
        return;
    }

    cout << left
         << setw(6) << "ID"
         << setw(18) << "Name"
         << setw(18) << "Current"
         << setw(18) << "Correct"
         << "Action\n";
    cout << string(76, '-') << "\n";

    for (const auto& item : misplaced) {
        cout << left
             << setw(6) << item.id
             << setw(18) << item.name.substr(0, 17)
             << setw(18) << item.currentLocation.substr(0, 17)
             << setw(18) << item.expectedLocation.substr(0, 17)
             << "Move to " << item.expectedLocation << "\n";
    }

    cout << "=============================\n\n";
}

static void printActionReport(const vector<ActionEntry>& actions) {
    cout << "\n====== ACTION REPORT ======\n";
    if (actions.empty()) {
        cout << "No action items generated.\n";
        cout << "===========================\n\n";
        return;
    }

    for (const auto& action : actions) {
        cout << "[" << action.type << "] "
             << action.itemName << " | "
             << action.priority << " | "
             << action.message << "\n";
    }
    cout << "===========================\n\n";
}

static void printSummary(const SummaryReport& summary) {
    cout << "\n====== SYSTEM SUMMARY ======\n";
    cout << "Total items              : " << summary.totalItems << "\n";
    cout << "Lost items count         : " << summary.lostItems << "\n";
    cout << "Misplaced items count    : " << summary.misplacedItems << "\n";
    cout << "High-risk items          : " << summary.highRiskItems << "\n";
    cout << "Estimated financial loss : $" << toStr(summary.estimatedFinancialLoss) << "\n";
    cout << "Recommended actions\n";
    for (const auto& action : summary.recommendedActions) {
        cout << "  - " << action << "\n";
    }
    cout << "============================\n\n";
}

class InventoryApplication {
public:
    InventoryApplication()
        : repositoryConfiguration_(ConfigService::instance().repositoryConfiguration()),
          repository_(RepositoryFactory::create(repositoryConfiguration_)),
          loginSystem_(repository_->loadUsers()),
          runtimeOptions_(loadRuntimeOptions()) {}

    int run() {
        AppLogger::initialize();
        printBanner();
        initialize();
        if (runtimeOptions_.serviceMode) {
            runServiceLoop();
            finalizeService();
        } else {
            runLoginLoop();
            finalize();
        }
        AppLogger::shutdown();
        return 0;
    }

private:
    string mode_;
    ItemList items_;
    AuditHistory history_;
    RepositoryConfiguration repositoryConfiguration_;
    unique_ptr<StorageRepository> repository_;
    LoginSystem loginSystem_;
    InventoryAnalysisService analysisService_{ConfigService::instance().data().topK};
    OptimizationEngine optimizationEngine_{ConfigService::instance().data().topK};
    RuntimeOptions runtimeOptions_;

    static bool isAdminRole(const string& role) {
        return role == "Admin";
    }

    static bool isWarehouseManagerRole(const string& role) {
        return role == "Warehouse Manager";
    }

    static bool isAnalystRole(const string& role) {
        return role == "Analyst";
    }

    static bool isViewerRole(const string& role) {
        return role == "Viewer";
    }

    void printBanner() const {
        cout << "\n============================================\n";
        cout << "      SmartInventory Smart System\n";
        cout << "============================================\n\n";
    }

    void initialize() {
        if (runtimeOptions_.serviceMode) {
            items_ = loadDatasetForModeNonInteractive(runtimeOptions_.startupMode, mode_, *repository_);
        } else {
            items_ = loadDatasetForMode(mode_, *repository_);
        }

        if (mode_ == "Demo") {
            seedDemoAudits(items_, history_);
            for (const auto& audit : history_.getAudits()) {
                repository_->saveAudit(audit);
            }
            if (!history_.getAudits().empty()) {
                items_ = history_.getAudits().back().items;
            }
        } else {
            history_.addAudit(timestampNow(), items_);
            const auto& audits = history_.getAudits();
            if (!audits.empty()) {
                repository_->saveAudit(audits.back());
            }
        }

        persistCurrentState();
        AppLogger::info("application_initialized",
                        {{"mode", mode_},
                         {"service_mode", runtimeOptions_.serviceMode ? "true" : "false"},
                         {"repository_backend", repositoryConfiguration_.backend == RepositoryBackend::PostgreSQL ? "postgresql" : "csv"}});
    }

    void persistCurrentState() {
        const int defaultBudget = ConfigService::instance().data().defaultBudget;
        AnalysisSnapshot snapshot = analysisService_.analyze(items_, history_, defaultBudget);
        writeJSON(mode_, items_, history_, snapshot, defaultBudget);
    }

    void runLoginLoop() {
        while (true) {
            cout << "Login\n";
            cout << "1. Sign in\n";
            cout << "2. Exit\n";
            int choice = promptInt("Choice: ", 1);
            if (choice == 2) {
                break;
            }

            UserSession session = authenticateUser();
            if (!session.authenticated) {
                continue;
            }

            routeSession(session);
        }
    }

    void runServiceLoop() {
        AppLogger::info("service_mode_started",
                        {{"mode", mode_},
                         {"refresh_interval_seconds", to_string(runtimeOptions_.refreshIntervalSeconds)},
                         {"run_once", runtimeOptions_.runOnce ? "true" : "false"}});

        do {
            persistCurrentState();
            AppLogger::info("service_iteration_completed",
                            {{"mode", mode_},
                             {"output_json_path", ConfigService::instance().data().outputJsonPath}});

            if (runtimeOptions_.runOnce) {
                return;
            }

            const int sleepSeconds = max(1, runtimeOptions_.refreshIntervalSeconds);
#ifdef _WIN32
            Sleep(static_cast<DWORD>(sleepSeconds * 1000));
#else
            std::this_thread::sleep_for(std::chrono::seconds(sleepSeconds));
#endif
        } while (true);
    }

    UserSession authenticateUser() const {
        string username = promptString("Username: ");
        string password = promptString("Password: ");
        UserSession session = loginSystem_.authenticate(username, password);

        if (!session.authenticated) {
            const auto& config = ConfigService::instance().data();
            cout << "Invalid credentials. Try one of: "
                 << config.adminUser.username << ", "
                 << config.warehouseManagerUser.username << ", "
                 << config.analystUser.username << ", "
                 << config.viewerUser.username << ".\n";
            return session;
        }

        cout << "Authenticated as " << session.role << ".\n";
        return session;
    }

    void routeSession(const UserSession& session) {
        if (isAdminRole(session.role)) {
            runAdminDashboard();
        } else if (isWarehouseManagerRole(session.role)) {
            runEmployeeDashboard();
        } else if (isAnalystRole(session.role)) {
            runAnalystDashboard();
        } else if (isViewerRole(session.role)) {
            runViewerDashboard();
        } else {
            runManagerDashboard();
        }
    }

    void runAdminDashboard() {
        while (true) {
            cout << "Admin Dashboard\n";
            cout << "1. Warehouse Operations\n";
            cout << "2. Intelligence Reports\n";
            cout << "3. Logout\n";

            int choice = promptInt("Choice: ", 1);
            if (choice == 1) {
                runEmployeeDashboard();
            } else if (choice == 2) {
                runManagerDashboard();
            } else if (choice == 3) {
                return;
            }
        }
    }

    void runEmployeeDashboard() {
        while (true) {
            const int defaultBudget = ConfigService::instance().data().defaultBudget;
            AnalysisSnapshot snapshot = analysisService_.analyze(items_, history_, defaultBudget);
            writeJSON(mode_, items_, history_, snapshot, defaultBudget);

            cout << "Employee Dashboard\n";
            cout << "1. Add item\n";
            cout << "2. Update stock\n";
            cout << "3. Perform audit\n";
            cout << "4. View misplaced items\n";
            cout << "5. View action report\n";
            cout << "6. Logout\n";

            int choice = promptInt("Choice: ", 1);
            if (choice == 1) addItemInteractive(items_);
            else if (choice == 2) updateStockInteractive(items_);
            else if (choice == 3) performAudit(items_, history_, *repository_);
            else if (choice == 4) printMisplacedItems(snapshot.misplacedItems);
            else if (choice == 5) printActionReport(snapshot.actionReport);
            else if (choice == 6) return;
        }
    }

    void runManagerDashboard() {
        while (true) {
            const auto& config = ConfigService::instance().data();
            AnalysisSnapshot snapshot = analysisService_.analyze(items_, history_, config.defaultBudget);
            writeJSON(mode_, items_, history_, snapshot, config.defaultBudget);

            cout << "Manager Dashboard\n";
            cout << "1. View mismatch report\n";
            cout << "2. View high-risk items\n";
            cout << "3. View DP restock plan\n";
            cout << "4. View graph clusters\n";
            cout << "5. View theft timing\n";
            cout << "6. View system summary report\n";
            cout << "7. View action report\n";
            cout << "8. Logout\n";

            int choice = promptInt("Choice: ", 1);
            if (choice == 1) {
                HashAlgo hashAlgo = optimizationEngine_.createHashAnalyzer(items_);
                hashAlgo.trackFrequency();
                hashAlgo.detectMismatches();
                hashAlgo.printReport();
            } else if (choice == 2) {
                HeapAlgo heapAlgo = optimizationEngine_.createHeapAnalyzer(items_);
                heapAlgo.extractTopK(config.topK);
                heapAlgo.printTopK(config.topK);
            } else if (choice == 3) {
                DPAlgo dpAlgo = optimizationEngine_.createDynamicProgrammingAnalyzer(items_, config.defaultBudget);
                dpAlgo.solve();
                dpAlgo.printDPTable();
                dpAlgo.printResult();
            } else if (choice == 4) {
                WarehouseRelationshipAnalyzer relationshipAnalyzer =
                    optimizationEngine_.createWarehouseRelationshipAnalyzer(items_);
                relationshipAnalyzer.findWarehouseClusters();
                relationshipAnalyzer.printWarehouseRelationships();
                relationshipAnalyzer.printWarehouseClusters();
            } else if (choice == 5) {
                printTheftTiming(snapshot);
            } else if (choice == 6) {
                printSummary(snapshot.summary);
            } else if (choice == 7) {
                printActionReport(snapshot.actionReport);
            } else if (choice == 8) {
                return;
            }
        }
    }

    void runAnalystDashboard() {
        while (true) {
            const auto& config = ConfigService::instance().data();
            AnalysisSnapshot snapshot = analysisService_.analyze(items_, history_, config.defaultBudget);
            writeJSON(mode_, items_, history_, snapshot, config.defaultBudget);

            cout << "Analyst Dashboard\n";
            cout << "1. View mismatch report\n";
            cout << "2. View high-risk items\n";
            cout << "3. View DP restock plan\n";
            cout << "4. View graph clusters\n";
            cout << "5. View theft timing\n";
            cout << "6. View system summary report\n";
            cout << "7. Logout\n";

            int choice = promptInt("Choice: ", 1);
            if (choice == 1) {
                HashAlgo hashAlgo = optimizationEngine_.createHashAnalyzer(items_);
                hashAlgo.trackFrequency();
                hashAlgo.detectMismatches();
                hashAlgo.printReport();
            } else if (choice == 2) {
                HeapAlgo heapAlgo = optimizationEngine_.createHeapAnalyzer(items_);
                heapAlgo.extractTopK(config.topK);
                heapAlgo.printTopK(config.topK);
            } else if (choice == 3) {
                DPAlgo dpAlgo = optimizationEngine_.createDynamicProgrammingAnalyzer(items_, config.defaultBudget);
                dpAlgo.solve();
                dpAlgo.printDPTable();
                dpAlgo.printResult();
            } else if (choice == 4) {
                WarehouseRelationshipAnalyzer relationshipAnalyzer =
                    optimizationEngine_.createWarehouseRelationshipAnalyzer(items_);
                relationshipAnalyzer.findWarehouseClusters();
                relationshipAnalyzer.printWarehouseRelationships();
                relationshipAnalyzer.printWarehouseClusters();
            } else if (choice == 5) {
                printTheftTiming(snapshot);
            } else if (choice == 6) {
                printSummary(snapshot.summary);
            } else if (choice == 7) {
                return;
            }
        }
    }

    void runViewerDashboard() {
        while (true) {
            const int defaultBudget = ConfigService::instance().data().defaultBudget;
            AnalysisSnapshot snapshot = analysisService_.analyze(items_, history_, defaultBudget);
            writeJSON(mode_, items_, history_, snapshot, defaultBudget);

            cout << "Viewer Dashboard\n";
            cout << "1. View system summary report\n";
            cout << "2. View action report\n";
            cout << "3. Logout\n";

            int choice = promptInt("Choice: ", 1);
            if (choice == 1) {
                printSummary(snapshot.summary);
            } else if (choice == 2) {
                printActionReport(snapshot.actionReport);
            } else if (choice == 3) {
                return;
            }
        }
    }

    void printTheftTiming(const AnalysisSnapshot& snapshot) const {
        cout << "\n====== THEFT TIMING ======\n";
        if (snapshot.theftTiming.empty()) {
            cout << "No mismatch timing could be derived from audit history.\n";
        } else {
            for (const auto& row : snapshot.theftTiming) {
                cout << row.message << "\n";
            }
        }
        cout << "==========================\n\n";
    }

    void finalize() {
        const int defaultBudget = ConfigService::instance().data().defaultBudget;
        AnalysisSnapshot finalSnapshot = analysisService_.analyze(items_, history_, defaultBudget);
        writeJSON(mode_, items_, history_, finalSnapshot, defaultBudget);
        printSummary(finalSnapshot.summary);
        cout << "Session closed.\n";
    }

    void finalizeService() {
        const int defaultBudget = ConfigService::instance().data().defaultBudget;
        AnalysisSnapshot finalSnapshot = analysisService_.analyze(items_, history_, defaultBudget);
        writeJSON(mode_, items_, history_, finalSnapshot, defaultBudget);
        AppLogger::info("service_mode_stopped",
                        {{"mode", mode_},
                         {"output_json_path", ConfigService::instance().data().outputJsonPath}});
    }
};

int main() {
    InventoryApplication app;
    return app.run();
}
