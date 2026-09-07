#pragma once

#include "../include/api_application_services.h"
#include "../include/password_hasher.h"

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace test_support {

inline Item makeItem(int id,
                     const std::string& name,
                     const std::string& category,
                     int expected,
                     int actual,
                     double price,
                     int demand,
                     const std::string& expectedLocation,
                     const std::string& currentLocation,
                     int frequency = 0,
                     int warehouseId = 1,
                     const std::string& warehouseName = "Main Warehouse") {
    return Item(id,
                name,
                category,
                expected,
                actual,
                price,
                demand,
                expectedLocation,
                currentLocation,
                frequency,
                warehouseId,
                warehouseName);
}

inline ItemList sampleItems() {
    return {
        makeItem(1, "Router", "Electronics", 100, 70, 12.5, 60, "Zone-A", "Zone-A", 2, 1, "North Hub"),
        makeItem(2, "Monitor", "Electronics", 80, 80, 25.0, 40, "Zone-A", "Zone-B", 1, 1, "North Hub"),
        makeItem(3, "Bolt", "Hardware", 150, 90, 4.0, 55, "Zone-C", "Zone-C", 3, 2, "South Hub"),
        makeItem(4, "Gloves", "Safety", 60, 65, 8.0, 20, "Zone-D", "Zone-D", 0, 2, "South Hub")
    };
}

inline WarehouseList sampleWarehouses() {
    return {
        Warehouse(1, "North Hub", "WH-N", "Delhi"),
        Warehouse(2, "South Hub", "WH-S", "Bengaluru")
    };
}

inline AuditHistory sampleAuditHistory() {
    AuditHistory history;

    ItemList auditOne = sampleItems();
    auditOne[0].actual = 100;
    auditOne[0].currentLocation = "Zone-A";
    auditOne[0].refreshDerived();
    auditOne[1].actual = 80;
    auditOne[1].currentLocation = "Zone-A";
    auditOne[1].refreshDerived();
    auditOne[2].actual = 150;
    auditOne[2].refreshDerived();

    ItemList auditTwo = auditOne;
    auditTwo[0].actual = 85;
    auditTwo[0].refreshDerived();
    auditTwo[2].actual = 120;
    auditTwo[2].refreshDerived();

    ItemList auditThree = sampleItems();

    history.addAudit("2026-08-05T08:00:00", auditOne);
    history.addAudit("2026-08-05T09:00:00", auditTwo);
    history.addAudit("2026-08-05T10:00:00", auditThree);
    return history;
}

inline std::vector<Audit> sampleAudits() {
    return sampleAuditHistory().getAudits();
}

inline std::vector<UserAccount> sampleUsers() {
    return {
        {"admin", PasswordHasher::hashPassword("admin-secret"), "Admin"},
        {"analyst", PasswordHasher::hashPassword("analyst-secret"), "Analyst"},
        {"viewer", PasswordHasher::hashPassword("viewer-secret"), "Viewer"}
    };
}

inline AnalysisSnapshot sampleSnapshot() {
    ItemList items = sampleItems();
    AuditHistory history = sampleAuditHistory();
    InventoryAnalysisService service(3);
    return service.analyze(items, history, 1500);
}

inline std::filesystem::path uniqueTempPath(const std::string& stem, const std::string& extension) {
    const auto base = std::filesystem::temp_directory_path();
    const auto unique = std::filesystem::path(stem + "-" + std::to_string(std::rand()) + extension);
    return base / unique;
}

inline std::filesystem::path writeTempFile(const std::string& stem,
                                           const std::string& extension,
                                           const std::string& contents) {
    const auto filePath = uniqueTempPath(stem, extension);
    std::ofstream output(filePath);
    output << contents;
    output.close();
    return filePath;
}

class FakeStorageRepository : public StorageRepository {
public:
    FakeStorageRepository(ItemList inventory = sampleItems(),
                          std::vector<Audit> audits = sampleAudits(),
                          std::vector<UserAccount> users = sampleUsers())
        : inventory_(std::move(inventory)),
          audits_(std::move(audits)),
          users_(std::move(users)) {}

    ItemList loadInventory() const override {
        return inventory_;
    }

    void saveAudit(const Audit& audit) override {
        audits_.push_back(audit);
    }

    std::vector<Audit> loadAudits() const override {
        return audits_;
    }

    std::vector<UserAccount> loadUsers() const override {
        return users_;
    }

    const std::vector<Audit>& audits() const {
        return audits_;
    }

private:
    ItemList inventory_;
    std::vector<Audit> audits_;
    std::vector<UserAccount> users_;
};

}  // namespace test_support
