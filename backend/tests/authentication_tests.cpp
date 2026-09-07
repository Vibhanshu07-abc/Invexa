#include "../include/rest_api_controllers.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace test_support;

TEST(AuthenticationTests, LoginSystemAuthenticatesKnownUsers) {
    LoginSystem login(sampleUsers());

    const auto success = login.authenticate("admin", "admin-secret");
    const auto failure = login.authenticate("admin", "wrong-secret");

    EXPECT_TRUE(success.authenticated);
    EXPECT_EQ(success.role, "Admin");
    EXPECT_FALSE(failure.authenticated);
}

TEST(AuthenticationTests, AuthenticationServiceUsesRepositoryAccounts) {
    FakeStorageRepository repository;
    InventoryQueryService queryService(repository);
    AuthenticationService authService(queryService);

    const auto success = authService.authenticate("analyst", "analyst-secret");
    const auto failure = authService.authenticate("viewer", "invalid");

    EXPECT_TRUE(success.authenticated);
    EXPECT_EQ(success.username, "analyst");
    EXPECT_FALSE(failure.authenticated);
}

TEST(AuthenticationTests, AuthenticationControllerReturnsHttpStyleStatusCodes) {
    FakeStorageRepository repository;
    InventoryQueryService queryService(repository);
    AuthenticationService authService(queryService);
    AuthenticationController controller(authService);

    const auto success = controller.login("viewer", "viewer-secret");
    const auto failure = controller.login("viewer", "bad-secret");

    EXPECT_EQ(success.statusCode, 200);
    EXPECT_TRUE(success.data["authenticated"].as_bool());
    EXPECT_EQ(failure.statusCode, 401);
    EXPECT_FALSE(failure.data["authenticated"].as_bool());
}
