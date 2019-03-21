CPPFLAGS=-std=c++11 -Wall -O3 -Wfatal-errors -L/gmp_install/lib -lgmp -lgmpxx 
CPPDFLAGS=-std=c++11 -g -Wfatal-errors

all: cmp mpz

stl: stlsizeof.cpp
	g++ -o stlsizeof stlsizeof.cpp $(CPPFLAGS) 

mem: containers.cpp 
	g++ -o containers containers.cpp $(CPPFLAGS) 

cmp: compare-exts.o Timer.o
	g++ -o ~/bin/compare-exts compare-exts.o Timer.o $(CPPFLAGS) 

mpz: compare-exts-mpz.o Timer.o
	g++ -o compare-solutions compare-exts-mpz.o Timer.o $(CPPFLAGS) 

compare-exts-mpz.o: compare-exts-mpz.cpp
	g++ -c compare-exts-mpz.cpp -o compare-exts-mpz.o $(CPPFLAGS) 

cnt: count-exts.o Timer.o
	g++ -o ~/bin/count-exts count-exts.o Timer.o $(CPPFLAGS) 

count-exts.o: count-exts.cpp
	g++ -c count-exts.cpp -o count-exts.o $(CPPFLAGS) 

Timer.o: Timer.cpp
	g++ -c Timer.cpp -o Timer.o $(CPPFLAGS) 

clean:
	rm -f *.o; rm -f ../UTILITY/*.o
