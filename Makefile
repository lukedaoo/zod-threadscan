CXX         ?= g++
CSTD        ?= c++23
RELEASE_FLAGS = -Wall -O3 -std=$(CSTD)
DEBUG_FLAGS   = -Wall -O0 -g -fsanitize=address,undefined -std=$(CSTD)
LDFLAGS       = -pthread
BUILD_FLAGS  ?= $(RELEASE_FLAGS)
ALL_FLAGS     = $(BUILD_FLAGS) $(LDFLAGS)
TARGET        ?= threadscan
SRC           ?= first.cpp

config.h:
	@echo "[make] copying config.def.h → config.h"
	@cp config.def.h config.h

clean:
	@echo "[make] removing old build"
	@rm -f $(TARGET)

clean-all:
	@echo "[make] removing old build and config.h"
	@rm -f $(TARGET)
	@rm -f config.h

build: config.h
	@echo "[make] compiling $(TARGET)"
	@$(CXX) $(ALL_FLAGS) -o $(TARGET) $(SRC)

debug:
	@$(MAKE) BUILD_FLAGS="$(DEBUG_FLAGS)" build

release:
	@$(MAKE) BUILD_FLAGS="$(RELEASE_FLAGS)" build

run: clean build
	@echo "[make] starting program"
	@./$(TARGET)
