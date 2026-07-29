CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2
OBJ      = Option08.o BinomialTreeModel02.o

all: lattice

lattice: main.o $(OBJ)
	$(CXX) $(CXXFLAGS) -o lattice main.o $(OBJ)

main.o: main.cpp BinomialTreeModel02.h BinLattice02.h Option08.h
	$(CXX) $(CXXFLAGS) -c main.cpp

Option08.o: Option08.cpp Option08.h BinomialTreeModel02.h BinLattice02.h
	$(CXX) $(CXXFLAGS) -c Option08.cpp

BinomialTreeModel02.o: BinomialTreeModel02.cpp BinomialTreeModel02.h
	$(CXX) $(CXXFLAGS) -c BinomialTreeModel02.cpp

# Correctness suite. Validates against Black-Scholes and put-call parity, not against itself.
test: tests/test_pricing.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) -o run_tests tests/test_pricing.cpp $(OBJ)
	./run_tests

# Wall-clock and memory comparison of the O(N^2) and O(N) American pricing paths.
bench: tests/bench.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) -o run_bench tests/bench.cpp $(OBJ)
	./run_bench

clean:
	rm -rf lattice run_tests run_bench *.o HW07

.PHONY: all test bench clean
