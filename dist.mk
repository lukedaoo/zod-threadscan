CXX      ?= g++
CXXFLAGS  = -Wall -O3 -std=c++23 -pthread

.PHONY: all run clean
.DEFAULT_GOAL := run

config: 
	@if [ ! -f config.h ]; then \
		echo "config.h missing, creating from defaults..."; \
		cp config.def.h config.h; \
	else \
		echo "using existing config.h"; \
	fi

all: config
	$(CXX) $(CXXFLAGS) -o threadscan first.cpp

run: all
	./threadscan

clean:
	rm -f threadscan config.h
