#pragma once

#ifndef AUDIT_REPOSITORY_H
#define AUDIT_REPOSITORY_H

#include "audit_history.h"

#include <vector>

class AuditRepository {
public:
    virtual ~AuditRepository() = default;

    virtual void saveAudit(const Audit& audit) = 0;
    virtual std::vector<Audit> loadAudits() const = 0;
};

#endif
