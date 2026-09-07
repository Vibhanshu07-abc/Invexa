#pragma once

#ifndef CSV_REPOSITORY_H
#define CSV_REPOSITORY_H

#include "csv_reader.h"
#include "storage_repository.h"

#include <string>
#include <vector>

class CSVRepository : public StorageRepository {
public:
    explicit CSVRepository(std::string inventoryPath);

    ItemList loadInventory() const override;
    void saveAudit(const Audit& audit) override;
    std::vector<Audit> loadAudits() const override;
    std::vector<UserAccount> loadUsers() const override;

private:
    std::string inventoryPath_;
    std::vector<Audit> audits_;
    std::vector<UserAccount> users_;
};

#endif
