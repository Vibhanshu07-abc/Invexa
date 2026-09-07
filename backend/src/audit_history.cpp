#include "../include/audit_history.h"
#include <algorithm>
#include <map>

using namespace std;

void AuditHistory::addAudit(const string& timestamp, const ItemList& items) {
    Audit audit;
    audit.timestamp = timestamp;
    audit.items = items;

    audits_.push_back(audit);
    sort(audits_.begin(), audits_.end(),
        [](const Audit& a, const Audit& b) {
            return a.timestamp < b.timestamp;
        });
}

const vector<Audit>& AuditHistory::getAudits() const {
    return audits_;
}

const Item* AuditHistory::findItem(const Audit& audit, int itemId) const {
    for (const auto& item : audit.items) {
        if (item.id == itemId) return &item;
    }
    return nullptr;
}

int AuditHistory::mismatchFrequency(int itemId) const {
    int count = 0;
    for (const auto& audit : audits_) {
        const Item* item = findItem(audit, itemId);
        if (item && (item->mismatch != 0 || item->isMisplaced())) count++;
    }
    return count;
}

bool AuditHistory::empty() const {
    return audits_.empty();
}

vector<TheftTimingRecord> AuditHistory::detectFirstMismatchMoments() const {
    vector<TheftTimingRecord> result;
    if (audits_.empty()) return result;

    map<int, string> names;
    for (const auto& audit : audits_) {
        for (const auto& item : audit.items) {
            names[item.id] = item.name;
        }
    }

    for (const auto& pair : names) {
        int itemId = pair.first;
        int low = 0;
        int high = (int)audits_.size() - 1;
        int found = -1;

        while (low <= high) {
            int mid = low + (high - low) / 2;
            const Item* item = findItem(audits_[mid], itemId);
            bool mismatch = item && item->mismatch > 0;
            if (mismatch) {
                found = mid;
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }

        if (found == -1) continue;

        TheftTimingRecord rec;
        rec.itemId = itemId;
        rec.itemName = pair.second;
        rec.mismatchAudit = audits_[found].timestamp;
        rec.previousAudit = found == 0 ? "Initial state" : audits_[found - 1].timestamp;
        rec.message = "Item " + rec.itemName + " was lost between " +
                      rec.previousAudit + " and " + rec.mismatchAudit;
        result.push_back(rec);
    }

    sort(result.begin(), result.end(),
        [](const TheftTimingRecord& a, const TheftTimingRecord& b) {
            if (a.previousAudit == b.previousAudit) return a.itemId < b.itemId;
            return a.previousAudit < b.previousAudit;
        });

    return result;
}
