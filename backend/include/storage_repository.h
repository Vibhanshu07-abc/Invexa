#pragma once

#ifndef STORAGE_REPOSITORY_H
#define STORAGE_REPOSITORY_H

#include "audit_repository.h"
#include "inventory_repository.h"
#include "user_repository.h"

class StorageRepository : public InventoryRepository,
                          public AuditRepository,
                          public UserRepository {
public:
    ~StorageRepository() override = default;
};

#endif
