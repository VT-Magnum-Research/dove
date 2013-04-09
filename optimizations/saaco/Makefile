all: bin/AntHybrid

bin/AntHybrid: utils	
	g++ -Wall -Isrc/utils -c src/mps.cpp -o build/mps.o
	g++ -Wall -Isrc/utils -c src/acomps.cpp -o build/acomps.o
	g++ -Wall -o bin/AntHybrid build/*.o

utils:
	g++ -Wall -c src/utils/ants.cpp -o build/ants.o
	g++ -Wall -c src/utils/graph.cpp -o build/graph.o
	g++ -Wall -c src/utils/localsearch.cpp -o build/localsearch.o
	g++ -Wall -c src/utils/util.cpp -o build/util.o

clean: .
	rm -f build/*.o
	rm -f bin/AntHybrid
