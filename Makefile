CC = g++
CFLAGS = -Wall -ggdb3 -std=c++11

HW07: main.o Option08.o BinomialTreeModel02.o
	$(CC) $(CFLAGS) -o HW07 main.o Option08.o BinomialTreeModel02.o

main.o: main.cpp BinomialTreeModel02.h BinLattice02.h Option08.h
	$(CC) $(CFLAGS) -c main.cpp

Option08.o: Option08.cpp Option08.h BinomialTreeModel02.h BinLattice02.h
	$(CC) $(CFLAGS) -c Option08.cpp

BinomialTreeModel02.o: BinomialTreeModel02.cpp BinomialTreeModel02.h
	$(CC) $(CFLAGS) -c BinomialTreeModel02.cpp

clean:
	rm -rf HW07 *.o
