#include "../include/csv_repository.h"
#include "../include/config_service.h"

using namespace std;

CSVRepository::CSVRepository(string inventoryPath)
    : inventoryPath_(std::move(inventoryPath)) {
    const auto& config = ConfigService::instance().data();
    users_.push_back({config.adminUser.username, config.adminUser.passwordHash, config.adminUser.role});
    users_.push_back({config.warehouseManagerUser.username, config.warehouseManagerUser.passwordHash, config.warehouseManagerUser.role});
    users_.push_back({config.analystUser.username, config.analystUser.passwordHash, config.analystUser.role});
    users_.push_back({config.viewerUser.username, config.viewerUser.passwordHash, config.viewerUser.role});
}

ItemList CSVRepository::loadInventory() const {
    CSVReader reader(inventoryPath_);
    return reader.load();
}

void CSVRepository::saveAudit(const Audit& audit) {
    audits_.push_back(audit);
}

vector<Audit> CSVRepository::loadAudits() const {
    return audits_;
}

vector<UserAccount> CSVRepository::loadUsers() const {
    return users_;
}
