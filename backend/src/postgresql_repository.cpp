#include "../include/postgresql_repository.h"
#include "../include/config_service.h"

using namespace std;

PostgreSQLRepository::PostgreSQLRepository(string csvImportPath,
                                           PostgreSQLConfiguration configuration)
    : csvImportPath_(std::move(csvImportPath)),
      configuration_(std::move(configuration)) {
    const auto& config = ConfigService::instance().data();
    users_.push_back({config.adminUser.username, config.adminUser.passwordHash, config.adminUser.role});
    users_.push_back({config.warehouseManagerUser.username, config.warehouseManagerUser.passwordHash, config.warehouseManagerUser.role});
    users_.push_back({config.analystUser.username, config.analystUser.passwordHash, config.analystUser.role});
    users_.push_back({config.viewerUser.username, config.viewerUser.passwordHash, config.viewerUser.role});
}

ItemList PostgreSQLRepository::loadInventory() const {
    if (cachedInventory_.empty()) {
        CSVReader reader(csvImportPath_);
        cachedInventory_ = reader.load();
    }

    return cachedInventory_;
}

void PostgreSQLRepository::saveAudit(const Audit& audit) {
    audits_.push_back(audit);
}

vector<Audit> PostgreSQLRepository::loadAudits() const {
    return audits_;
}

vector<UserAccount> PostgreSQLRepository::loadUsers() const {
    return users_;
}
