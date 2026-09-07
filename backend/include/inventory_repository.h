#pragma once

#ifndef INVENTORY_REPOSITORY_H
#define INVENTORY_REPOSITORY_H

#include "item.h"

class InventoryRepository {
public:
    virtual ~InventoryRepository() = default;

    virtual ItemList loadInventory() const = 0;
};

#endif
