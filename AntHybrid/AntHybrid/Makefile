all: bin/acotsp

bin/acotsp: build/acotsp.o build/tsp.o
	g++ build/acotsp.o build/tsp.o -L./lib/ -laco -o bin/acotsp

build/acotsp.o: src/acotsp.cpp
	g++ -Wall -ansi -I include/ -c src/acotsp.cpp -o build/acotsp.o

build/tsp.o: src/tsp.cpp
	g++ -Wall -ansi -I include/ -c src/tsp.cpp -o build/tsp.o

clean: .
	rm bin/acotsp
