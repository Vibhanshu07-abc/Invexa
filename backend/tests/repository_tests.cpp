#include "../include/csv_reader.h"
#include "../include/csv_repository.h"
#include "../include/postgresql_repository.h"
#include "../include/repository_configuration.h"
#include "../include/repository_factory.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <filesystem>

using namespace test_support;

namespace {

std::string validCsvContents() {
    return "id,name,category,expected,actual,price,demand,expectedLocation,currentLocation,restockCost,frequency,warehouseId,warehouseName\n"
           "1,Router,Electronics,100,70,12.5,60,Zone-A,Zone-A,100,2,1,North Hub\n"
           "2,Monitor,Electronics,80,80,25.0,40,Zone-A,Zone-B,60,1,1,North Hub\n";
}

}  // namespace

TEST(RepositoryTests, CsvReaderLoadsValidRowsAndSkipsInvalidOnes) {
    const auto csvPath = writeTempFile(
        "smartinventory-reader",
        ".csv",
        validCsvContents() + "2,Duplicate,Electronics,20,20,2.0,5,Zone-A,Zone-A,10,0,1,North Hub\n");

    CSVReader reader(csvPath.string());
    const auto items = reader.load();

    EXPECT_EQ(items.size(), 2);
    EXPECT_EQ(reader.skippedRows(), 1);
    EXPECT_FALSE(reader.getErrors().empty());

    std::filesystem::remove(csvPath);
}

TEST(RepositoryTests, CsvRepositoryLoadsInventoryAndPersistsAuditsInMemory) {
    const auto csvPath = writeTempFile("smartinventory-csv-repo", ".csv", validCsvContents());
    CSVRepository repository(csvPath.string());
    const Audit audit{"2026-08-05T11:00:00", sampleItems()};

    const auto items = repository.loadInventory();
    repository.saveAudit(audit);

    EXPECT_EQ(items.size(), 2);
    ASSERT_EQ(repository.loadAudits().size(), 1);
    EXPECT_EQ(repository.loadAudits().front().timestamp, audit.timestamp);
    EXPECT_GE(repository.loadUsers().size(), 4);

    std::filesystem::remove(csvPath);
}

TEST(RepositoryTests, PostgreSqlRepositoryCachesInventoryAfterFirstLoad) {
    const auto csvPath = writeTempFile("smartinventory-postgres-repo", ".csv", validCsvContents());
    PostgreSQLConfiguration configuration;
    PostgreSQLRepository repository(csvPath.string(), configuration);

    const auto firstLoad = repository.loadInventory();
    std::ofstream overwrite(csvPath);
    overwrite << "id,name,category,expected,actual,price,demand,expectedLocation,currentLocation,restockCost,frequency,warehouseId,warehouseName\n";
    overwrite.close();
    const auto secondLoad = repository.loadInventory();

    EXPECT_EQ(firstLoad.size(), 2);
    EXPECT_EQ(secondLoad.size(), 2);

    std::filesystem::remove(csvPath);
}

TEST(RepositoryTests, RepositoryFactoryCreatesExpectedRepositoryType) {
    RepositoryConfiguration csvConfiguration;
    csvConfiguration.backend = RepositoryBackend::CSV;
    csvConfiguration.csvPath = "ignored.csv";

    RepositoryConfiguration postgresConfiguration = csvConfiguration;
    postgresConfiguration.backend = RepositoryBackend::PostgreSQL;

    auto csvRepository = RepositoryFactory::create(csvConfiguration);
    auto postgresRepository = RepositoryFactory::create(postgresConfiguration);

    EXPECT_NE(dynamic_cast<CSVRepository*>(csvRepository.get()), nullptr);
    EXPECT_NE(dynamic_cast<PostgreSQLRepository*>(postgresRepository.get()), nullptr);
}

TEST(RepositoryTests, RepositoryConfigurationLoaderParsesConfigFile) {
    const auto configPath = writeTempFile(
        "smartinventory-repository-config",
        ".conf",
        "backend = postgresql\n"
        "csv_path = ./inventory.csv\n"
        "postgres_host = db.internal\n"
        "postgres_port = 5544\n"
        "postgres_database = smart\n"
        "postgres_username = svc\n"
        "postgres_password = secret\n");

    RepositoryConfigurationLoader loader(configPath.string(), "./fallback.csv");
    const auto configuration = loader.load();

    EXPECT_EQ(configuration.backend, RepositoryBackend::PostgreSQL);
    EXPECT_EQ(configuration.csvPath, "./inventory.csv");
    EXPECT_EQ(configuration.postgresql.host, "db.internal");
    EXPECT_EQ(configuration.postgresql.port, 5544);
    EXPECT_EQ(configuration.postgresql.database, "smart");
    EXPECT_EQ(configuration.postgresql.username, "svc");
    EXPECT_EQ(configuration.postgresql.password, "secret");

    std::filesystem::remove(configPath);
}
