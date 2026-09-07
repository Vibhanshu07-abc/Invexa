#pragma once

#ifndef POSTGRESQL_REPOSITORY_H
#define POSTGRESQL_REPOSITORY_H

#include "csv_reader.h"
#include "repository_configuration.h"
#include "storage_repository.h"

#include <string>
#include <vector>

class PostgreSQLRepository : public StorageRepository {
public:
    PostgreSQLRepository(std::string csvImportPath, PostgreSQLConfiguration configuration);

    ItemList loadInventory() const override;
    void saveAudit(const Audit& audit) override;
    std::vector<Audit> loadAudits() const override;
    std::vector<UserAccount> loadUsers() const override;

private:
    std::string csvImportPath_;
    PostgreSQLConfiguration configuration_;
    mutable ItemList cachedInventory_;
    std::vector<Audit> audits_;
    std::vector<UserAccount> users_;
};

#endif
