# NTHU-Bike final project build.
#
#   make            build ./bin/main
#   make run        build, then run one case/version (override on the CLI)
#   make clean      remove build artifacts
#
# Examples:
#   make run case=case1 version=basic
#   make run case=case3 version=advanced

CXX      := g++
CXXFLAGS := -std=c++11 -O2 -Wall
SRC      := src/main.cpp src/common.cpp src/basic.cpp src/advanced.cpp
BIN      := bin/main

case    ?= case1
version ?= basic

$(BIN): $(SRC) src/nthu_bike.h
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $(BIN) $(SRC)

.PHONY: run clean
run: $(BIN)
	@mkdir -p result/$(case)
	./$(BIN) $(case) $(version)

clean:
	rm -rf bin/main bin/*.dSYM
