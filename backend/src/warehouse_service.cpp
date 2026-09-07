#include "../include/warehouse_service.h"

#include <algorithm>
#include <cctype>

using namespace std;

namespace {
string trimCopy(const string& value) {
    const auto start = find_if_not(value.begin(), value.end(),
        [](unsigned char ch) { return isspace(ch) != 0; });
    const auto end = find_if_not(value.rbegin(), value.rend(),
        [](unsigned char ch) { return isspace(ch) != 0; }).base();

    if (start >= end) {
        return "";
    }

    return string(start, end);
}
}

bool WarehouseService::createWarehouse(WarehouseList& warehouses, const Warehouse& warehouse) const {
    if (!isValidWarehouse(warehouse) ||
        warehouseExists(warehouses, warehouse.id) ||
        warehouseCodeExists(warehouses, warehouse.code, warehouse.id)) {
        return false;
    }

    warehouses.push_back(warehouse);
    return true;
}

bool WarehouseService::updateWarehouse(WarehouseList& warehouses, const Warehouse& warehouse) const {
    if (!isValidWarehouse(warehouse) ||
        warehouseCodeExists(warehouses, warehouse.code, warehouse.id)) {
        return false;
    }

    const auto existing = findWarehouse(warehouses, warehouse.id);
    if (existing == warehouses.end()) {
        return false;
    }

    existing->name = warehouse.name;
    existing->code = warehouse.code;
    existing->location = warehouse.location;
    return true;
}

bool WarehouseService::deleteWarehouse(WarehouseList& warehouses,
                                       const ItemList& items,
                                       int warehouseId) const {
    if (warehouseId <= 0) {
        return false;
    }

    const auto existing = findWarehouse(warehouses, warehouseId);
    if (existing == warehouses.end()) {
        return false;
    }

    for (const auto& item : items) {
        if (item.warehouseId == warehouseId) {
            return false;
        }
    }

    warehouses.erase(existing);
    return true;
}

bool WarehouseService::assignInventoryToWarehouse(ItemList& items,
                                                  const WarehouseList& warehouses,
                                                  int itemId,
                                                  int warehouseId) const {
    const auto warehouse = findWarehouse(warehouses, warehouseId);
    if (warehouse == warehouses.end()) {
        return false;
    }

    const auto item = findItem(items, itemId);
    if (item == items.end()) {
        return false;
    }

    item->setWarehouseOwnership(*warehouse);
    return true;
}

bool WarehouseService::assignInventoryToWarehouse(ItemList& items,
                                                  const WarehouseList& warehouses,
                                                  const vector<int>& itemIds,
                                                  int warehouseId) const {
    const auto warehouse = findWarehouse(warehouses, warehouseId);
    if (warehouse == warehouses.end() || itemIds.empty()) {
        return false;
    }

    bool updated = false;
    for (int itemId : itemIds) {
        const auto item = findItem(items, itemId);
        if (item == items.end()) {
            continue;
        }

        item->setWarehouseOwnership(*warehouse);
        updated = true;
    }

    return updated;
}

bool WarehouseService::isValidWarehouse(const Warehouse& warehouse) const {
    return warehouse.id > 0 &&
           !trimCopy(warehouse.name).empty() &&
           !trimCopy(warehouse.code).empty() &&
           !trimCopy(warehouse.location).empty();
}

bool WarehouseService::warehouseExists(const WarehouseList& warehouses, int warehouseId) const {
    return findWarehouse(warehouses, warehouseId) != warehouses.end();
}

bool WarehouseService::warehouseCodeExists(const WarehouseList& warehouses,
                                           const string& code,
                                           int excludedWarehouseId) const {
    const string normalizedCode = trimCopy(code);
    return any_of(warehouses.begin(), warehouses.end(),
        [&](const Warehouse& warehouse) {
            return warehouse.id != excludedWarehouseId &&
                   trimCopy(warehouse.code) == normalizedCode;
        });
}

WarehouseList::iterator WarehouseService::findWarehouse(WarehouseList& warehouses,
                                                        int warehouseId) const {
    return find_if(warehouses.begin(), warehouses.end(),
        [&](const Warehouse& warehouse) {
            return warehouse.id == warehouseId;
        });
}

WarehouseList::const_iterator WarehouseService::findWarehouse(const WarehouseList& warehouses,
                                                              int warehouseId) const {
    return find_if(warehouses.begin(), warehouses.end(),
        [&](const Warehouse& warehouse) {
            return warehouse.id == warehouseId;
        });
}

ItemList::iterator WarehouseService::findItem(ItemList& items, int itemId) const {
    return find_if(items.begin(), items.end(),
        [&](const Item& item) {
            return item.id == itemId;
        });
}
