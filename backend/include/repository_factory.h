#pragma once

#ifndef REPOSITORY_FACTORY_H
#define REPOSITORY_FACTORY_H

#include "repository_configuration.h"
#include "storage_repository.h"

#include <memory>

class RepositoryFactory {
public:
    static std::unique_ptr<StorageRepository> create(const RepositoryConfiguration& configuration);
};

#endif
