all: bin/acotsp

bin/acotsp: libaco liblocalsearch build/acotsp.o build/tsp.o
	cp ../../libaco/trunk/build/libaco.a lib/
	cp ../../liblocalsearch/trunk/build/liblocalsearch.a lib/
	g++ -static build/acotsp.o build/tsp.o -L./lib/ -laco -o bin/acotsp

build/acotsp.o: src/acotsp.cpp include/acotsp/tsp.h include/libaco/ants.h include/libaco/graph.h include/libaco/util.h
	g++ -Wall -ansi -I include/ -c src/acotsp.cpp -o build/acotsp.o

build/tsp.o: src/tsp.cpp include/libaco/ants.h include/libaco/graph.h include/libaco/util.h include/liblocalsearch/localsearch.h
	g++ -Wall -ansi -I include/ -c src/tsp.cpp -o build/tsp.o

include/libaco/%.h: ../../libaco/trunk/include/libaco/%.h
	cp ../../libaco/trunk/include/libaco/$*.h include/libaco/

include/liblocalsearch/%.h: ../../liblocalsearch/trunk/include/liblocalsearch/%.h
	cp ../../liblocalsearch/trunk/include/liblocalsearch/$*.h include/liblocalsearch/

libaco:
	make -C ../../libaco/trunk -f ../../libaco/trunk/Makefile

liblocalsearch:
	make -C ../../liblocalsearch/trunk -f ../../liblocalsearch/trunk/Makefile

clean: .
	rm include/libaco/*.h build/*.o bin/acotsp
