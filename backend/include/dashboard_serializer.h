#pragma once

#ifndef DASHBOARD_SERIALIZER_H
#define DASHBOARD_SERIALIZER_H

#include "audit_history.h"
#include "inventory_analysis_service.h"
#include "item.h"
#include "report_serializer.h"

#include <nlohmann/json.hpp>
#include <string>

class DashboardSerializer {
public:
    nlohmann::json serialize(const std::string& generatedAt,
                             const std::string& mode,
                             int budget,
                             const ItemList& items,
                             const AuditHistory& history,
                             const AnalysisSnapshot& snapshot) const;

private:
    ReportSerializer reportSerializer_;
};

#endif
