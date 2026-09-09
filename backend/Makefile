CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I./include
LDFLAGS  =
LDLIBS   =

SRC_DIR      = src
OBJ_DIR      = build
TEST_DIR     = tests
TARGET       = SmartInventory
TEST_TARGET  = SmartInventoryTests
COVERAGE_DIR = coverage

PKG_CONFIG := $(shell command -v pkg-config 2>/dev/null)
ifeq ($(PKG_CONFIG),)
SPDLOG_FLAGS :=
SPDLOG_LIBS :=
GTEST_FLAGS :=
GTEST_LIBS := -lgtest -lgtest_main -pthread
else
SPDLOG_FLAGS := $(shell pkg-config --cflags spdlog 2>/dev/null)
SPDLOG_LIBS := $(shell pkg-config --libs spdlog 2>/dev/null)
GTEST_FLAGS := $(shell pkg-config --cflags gtest gtest_main 2>/dev/null)
GTEST_LIBS := $(shell pkg-config --libs gtest gtest_main 2>/dev/null)
ifeq ($(strip $(GTEST_LIBS)),)
GTEST_LIBS := -lgtest -lgtest_main -pthread
endif
endif

CXXFLAGS += $(SPDLOG_FLAGS)
LDLIBS += $(SPDLOG_LIBS)
CXXFLAGS += $(GTEST_FLAGS)

ifeq ($(COVERAGE),1)
CXXFLAGS += -O0 --coverage
LDFLAGS += --coverage
endif

ifeq ($(OS),Windows_NT)
MKDIR_P     = if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
MKDIR_TESTS = if not exist $(OBJ_DIR)\tests mkdir $(OBJ_DIR)\tests
MKDIR_COV   = if not exist $(COVERAGE_DIR) mkdir $(COVERAGE_DIR)
RM_RF       = if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR) & if exist $(TARGET).exe del /q $(TARGET).exe & if exist $(TARGET) del /q $(TARGET) & if exist $(TEST_TARGET).exe del /q $(TEST_TARGET).exe & if exist $(TEST_TARGET) del /q $(TEST_TARGET)
RUN_CMD     = .\\$(TARGET).exe
TEST_CMD    = .\\$(TEST_TARGET).exe
else
MKDIR_P     = mkdir -p $(OBJ_DIR)
MKDIR_TESTS = mkdir -p $(OBJ_DIR)/tests
MKDIR_COV   = mkdir -p $(COVERAGE_DIR)
RM_RF       = rm -rf $(OBJ_DIR) $(TARGET) $(TEST_TARGET) $(COVERAGE_DIR)
RUN_CMD     = ./$(TARGET)
TEST_CMD    = ./$(TEST_TARGET)
endif

SOURCES  = $(SRC_DIR)/main.cpp \
           $(SRC_DIR)/config_service.cpp \
           $(SRC_DIR)/csv_reader.cpp \
           $(SRC_DIR)/csv_repository.cpp \
           $(SRC_DIR)/postgresql_repository.cpp \
           $(SRC_DIR)/repository_configuration.cpp \
           $(SRC_DIR)/repository_factory.cpp \
           $(SRC_DIR)/hash_algo.cpp \
           $(SRC_DIR)/heap_algo.cpp \
           $(SRC_DIR)/greedy_algo.cpp \
           $(SRC_DIR)/dp_algo.cpp \
           $(SRC_DIR)/graph_algo.cpp \
           $(SRC_DIR)/audit_history.cpp \
           $(SRC_DIR)/optimization_engine.cpp \
           $(SRC_DIR)/inventory_analysis_service.cpp \
           $(SRC_DIR)/api_application_services.cpp \
           $(SRC_DIR)/decision_engine.cpp \
           $(SRC_DIR)/gemini_service.cpp \
           $(SRC_DIR)/ai_warehouse_assistant.cpp \
           $(SRC_DIR)/rest_api_controllers.cpp \
           $(SRC_DIR)/warehouse_health_service.cpp \
           $(SRC_DIR)/warehouse_relationship_analyzer.cpp \
           $(SRC_DIR)/warehouse_service.cpp \
           $(SRC_DIR)/report_serializer.cpp \
           $(SRC_DIR)/dashboard_serializer.cpp \
           $(SRC_DIR)/monitoring_registry.cpp \
           $(SRC_DIR)/logging.cpp \
           $(SRC_DIR)/password_hasher.cpp \
           $(SRC_DIR)/login.cpp

OBJECTS       = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
CORE_SOURCES  = $(filter-out $(SRC_DIR)/main.cpp, $(SOURCES))
CORE_OBJECTS  = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.test.o, $(CORE_SOURCES))
TEST_SOURCES  = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJECTS  = $(patsubst $(TEST_DIR)/%.cpp, $(OBJ_DIR)/tests/%.o, $(TEST_SOURCES))

.PHONY: all clean run test unit-test coverage

all: $(OBJ_DIR) $(TARGET)

$(OBJ_DIR):
	$(MKDIR_P)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	@echo "\n[Build] Binary ready: ./$(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.test.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.cpp
	$(MKDIR_TESTS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_TARGET): $(OBJ_DIR) $(CORE_OBJECTS) $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(CORE_OBJECTS) $(TEST_OBJECTS) $(LDLIBS) $(GTEST_LIBS)
	@echo "\n[Build] Test binary ready: ./$(TEST_TARGET)"

unit-test: $(TEST_TARGET)
	$(TEST_CMD)

test: unit-test

coverage: clean
	$(MAKE) COVERAGE=1 all unit-test
	$(MKDIR_COV)
	gcovr --root . --exclude "$(TEST_DIR)" --xml "$(COVERAGE_DIR)/coverage.xml" --html-details "$(COVERAGE_DIR)/index.html" --print-summary

run: all
	@echo "\n[Run] Executing SmartInventory...\n"
	$(RUN_CMD)

clean:
	$(RM_RF)
	@echo "[Clean] Build artefacts removed."
