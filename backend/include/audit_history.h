#pragma once
#ifndef AUDIT_HISTORY_H
#define AUDIT_HISTORY_H

#include "item.h"
#include <string>
#include <vector>

struct Audit {
    std::string timestamp;
    std::vector<Item> items;
};

struct TheftTimingRecord {
    int itemId;
    std::string itemName;
    std::string previousAudit;
    std::string mismatchAudit;
    std::string message;
};

class AuditHistory {
public:
    void addAudit(const std::string& timestamp, const ItemList& items);
    const std::vector<Audit>& getAudits() const;
    std::vector<TheftTimingRecord> detectFirstMismatchMoments() const;
    int mismatchFrequency(int itemId) const;
    bool empty() const;

private:
    std::vector<Audit> audits_;

    const Item* findItem(const Audit& audit, int itemId) const;
};

#endif
