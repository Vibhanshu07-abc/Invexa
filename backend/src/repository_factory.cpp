#include "../include/repository_factory.h"

#include "../include/csv_repository.h"
#include "../include/postgresql_repository.h"

using namespace std;

unique_ptr<StorageRepository> RepositoryFactory::create(const RepositoryConfiguration& configuration) {
    if (configuration.backend == RepositoryBackend::PostgreSQL) {
        return make_unique<PostgreSQLRepository>(configuration.csvPath, configuration.postgresql);
    }

    return make_unique<CSVRepository>(configuration.csvPath);
}
