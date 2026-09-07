#pragma once

#ifndef WAREHOUSE_SERVICE_H
#define WAREHOUSE_SERVICE_H

#include "item.h"

#include <string>
#include <vector>

class WarehouseService {
public:
    bool createWarehouse(WarehouseList& warehouses, const Warehouse& warehouse) const;
    bool updateWarehouse(WarehouseList& warehouses, const Warehouse& warehouse) const;
    bool deleteWarehouse(WarehouseList& warehouses, const ItemList& items, int warehouseId) const;
    bool assignInventoryToWarehouse(ItemList& items,
                                    const WarehouseList& warehouses,
                                    int itemId,
                                    int warehouseId) const;
    bool assignInventoryToWarehouse(ItemList& items,
                                    const WarehouseList& warehouses,
                                    const std::vector<int>& itemIds,
                                    int warehouseId) const;

private:
    bool isValidWarehouse(const Warehouse& warehouse) const;
    bool warehouseExists(const WarehouseList& warehouses, int warehouseId) const;
    bool warehouseCodeExists(const WarehouseList& warehouses,
                             const std::string& code,
                             int excludedWarehouseId) const;
    WarehouseList::iterator findWarehouse(WarehouseList& warehouses, int warehouseId) const;
    WarehouseList::const_iterator findWarehouse(const WarehouseList& warehouses, int warehouseId) const;
    ItemList::iterator findItem(ItemList& items, int itemId) const;
};

#endif
