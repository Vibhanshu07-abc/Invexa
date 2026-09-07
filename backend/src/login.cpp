#include "../include/login.h"
#include "../include/config_service.h"
#include "../include/password_hasher.h"

using namespace std;

LoginSystem::LoginSystem() {
    const auto& config = ConfigService::instance().data();
    accounts_.push_back({config.adminUser.username, config.adminUser.passwordHash, config.adminUser.role});
    accounts_.push_back({config.warehouseManagerUser.username, config.warehouseManagerUser.passwordHash, config.warehouseManagerUser.role});
    accounts_.push_back({config.analystUser.username, config.analystUser.passwordHash, config.analystUser.role});
    accounts_.push_back({config.viewerUser.username, config.viewerUser.passwordHash, config.viewerUser.role});
}

LoginSystem::LoginSystem(const vector<UserAccount>& accounts)
    : accounts_(accounts) {}

UserSession LoginSystem::authenticate(const string& username,
                                      const string& password) const {
    for (const auto& account : accounts_) {
        if (account.username == username &&
            PasswordHasher::verifyPassword(password, account.passwordHash)) {
            return {account.username, account.role, true};
        }
    }
    return {"", "", false};
}

const vector<UserAccount>& LoginSystem::accounts() const {
    return accounts_;
}
