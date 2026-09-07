#pragma once
#ifndef LOGIN_H
#define LOGIN_H

#include <string>
#include <vector>

struct UserAccount {
    std::string username;
    std::string passwordHash;
    std::string role;
};

struct UserSession {
    std::string username;
    std::string role;
    bool authenticated;
};

class LoginSystem {
public:
    LoginSystem();
    explicit LoginSystem(const std::vector<UserAccount>& accounts);

    UserSession authenticate(const std::string& username,
                             const std::string& password) const;
    const std::vector<UserAccount>& accounts() const;

private:
    std::vector<UserAccount> accounts_;
};

#endif
