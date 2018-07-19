PREFIX=/usr/local

catflap: CArgs.o  CatFlap.o  CCaetla.o
	g++ -g -O2 CArgs.o  CatFlap.o  CCaetla.o -o catflap

clean:
	rm *.o
	rm catflap

install: catflap
	cp catflap $(PREFIX)/bin

CArgs.o: CArgs.cpp
	g++ -g -O2 -c CArgs.cpp -o CArgs.o

CatFlap.o: CatFlap.cpp
	g++ -g -O2 -c CatFlap.cpp -o CatFlap.o  

CCaetla.o: CCaetla.cpp
	g++ -g -O2 -c CCaetla.cpp -o CCaetla.o  

