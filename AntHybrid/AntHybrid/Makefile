all: bin/AntHybrid

bin/AntHybrid: libaco liblocalsearch bin/mps.o bin/acomps.o
	g++ bin/mps.o bin/acomps.o -Llib/ -laco -o bin/AntHybrid

bin/acomps.o: src/acomps.cpp include/acomps/mps.h include/libaco/ants.h include/libaco/graph.h include/libaco/util.h include/mps.h
	g++ -Wall -ansi -I include/ -c src/acomps.cpp -o bin/acomps.o

acomps/%.h:
	cp src/$*.h include/

bin/utils:
	g++ -Wall -ansi -I include/ -c src/mps.cpp -o bin/mps.o

clean: .
	rm bin/*
