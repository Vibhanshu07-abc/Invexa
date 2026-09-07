#pragma once

#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include "login.h"

#include <vector>

class UserRepository {
public:
    virtual ~UserRepository() = default;

    virtual std::vector<UserAccount> loadUsers() const = 0;
};

#endif
