all: bin/AntHybrid

bin/AntHybrid: utils	
	g++  -Isrc/utils -c src/mps.cpp -o build/mps.o
	g++  -Isrc/utils -c src/acomps.cpp -o build/acomps.o
	g++  -o bin/AntHybrid build/*.o

utils:
	g++  -c src/utils/ants.cpp -o build/ants.o
	g++  -c src/utils/graph.cpp -o build/graph.o
	g++  -c src/utils/localsearch.cpp -o build/localsearch.o
	g++  -c src/utils/util.cpp -o build/util.o

clean: .
	rm build/*.o
	rm bin/AntHybrid
