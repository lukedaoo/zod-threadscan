.PHONY: all run build debug release clean clean-all test dist

CXX          ?= g++
CSTD         ?= c++23
RELEASE_FLAGS = -Wall -O3 -std=$(CSTD)
DEBUG_FLAGS   = -Wall -O0 -g -fsanitize=address,undefined -std=$(CSTD)
LDFLAGS       = -pthread
BUILD_FLAGS  ?= $(RELEASE_FLAGS)
ALL_FLAGS     = $(BUILD_FLAGS) $(LDFLAGS)
TARGET       ?= threadscan
SRC          ?= first.cpp
DIST_DIR      = dist

all: run

clean:
	@echo "[make] removing old build"
	@rm -f $(TARGET)

config: 
	@if [ ! -f config.h ]; then \
		echo "[make] config.h missing, creating from defaults..."; \
		cp config.def.h config.h; \
	else \
		echo "[make] using existing config.h"; \
	fi

build: config
	@echo "[make] compiling $(TARGET)"
	@$(CXX) $(ALL_FLAGS) -o $(TARGET) $(SRC)

debug:
	@$(MAKE) BUILD_FLAGS="$(DEBUG_FLAGS)" build

release:
	@$(MAKE) BUILD_FLAGS="$(RELEASE_FLAGS)" build

run: release
	@echo "[make] starting program"
	@./$(TARGET)

run-single-thread: release
	@./$(TARGET) --path $(PWD)/RandomFiles --word quasi --num-of-files 250 --num-of-threads 0 --no-console

run-multiple-thread: release
	@./$(TARGET) --path $(PWD)/RandomFiles --word quasi --num-of-files 250 --num-of-threads 5 --no-console
TEST_FLAGS  = -Wall -O0 -g -std=$(CSTD)
GTEST_LIBS  = -DGTEST_HAS_PTHREAD=1 -lgtest_main -lgtest -lpthread
TEST_SRCS   = $(wildcard tests/test_*.cpp)
TEST_BIN    = tests/test_all

test: config.h
	@echo "[make] compiling tests"
	$(CXX) $(TEST_FLAGS) -I. -o $(TEST_BIN) $(TEST_SRCS) $(GTEST_LIBS)
	@./$(TEST_BIN) --gtest_brief=1 --gtest_print_time=0

clean-all:
	@echo "[make] removing old build, config.h and dist"
	@rm -f $(TARGET)
	@rm -f config.h
	@rm -rf $(DIST_DIR)


DIST_SOURCES = first.cpp \
               threadscan_core.h threadscan_core_impl.cpp \
               threadscan_input.h threadscan_input_impl.cpp \
               threadscan_types.h \
               config.def.h LICENSE

dist:
	@echo "[make] creating $(DIST_DIR)/"
	@mkdir -p $(DIST_DIR)
	@cp $(DIST_SOURCES) $(DIST_DIR)/
	@cp dist.mk $(DIST_DIR)/Makefile
	@cp dist.readme.md $(DIST_DIR)/README.md
	@echo "[make] dist ready: $(DIST_DIR)/"

