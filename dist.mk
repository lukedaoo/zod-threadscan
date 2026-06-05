CXX      ?= g++
CXXFLAGS  = -Wall -O3 -std=c++23 -pthread
TARGET    = WordCount 

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
	$(CXX) $(CXXFLAGS) -o $(TARGET) first.cpp

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) config.h
