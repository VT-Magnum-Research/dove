all: bin/AntHybrid

bin/AntHybrid: libaco liblocalsearch bin/mps.o bin/acomps.o
	g++ bin/mps.o bin/acomps.o -Llib/ -laco -o bin/AntHybrid

bin/acomps.o: src/acomps.cpp include/acomps/mps.h include/libaco/ants.h include/libaco/graph.h include/libaco/util.h include/mps.h
	g++ -Wall -ansi -I include/ -c src/acomps.cpp -o bin/acomps.o

include/acomps/%.h: 
	cp src/$*.h include/

bin/mps.o: include/libaco/ants.h include/libaco/graph.h include/libaco/util.h include/liblocalsearch/localsearch.h include/acomps/mps.h
	g++ -Wall -ansi -I include/ -c src/mps.cpp -o bin/mps.o

include/libaco/%.h: lib/libaco/include/libaco/%.h
	cp lib/libaco/include/libaco/$*.h include/libaco/
	cp lib/libaco/include/libaco/$*.h include/

include/liblocalsearch/%.h: lib/liblocalsearch/include/liblocalsearch/%.h
	cp lib/liblocalsearch/include/liblocalsearch/$*.h include/liblocalsearch/
	cp lib/liblocalsearch/include/liblocalsearch/$*.h include/

libaco:
	make -C lib/libaco -f Makefile

liblocalsearch:
	make -C lib/liblocalsearch -f Makefile

clean: .
	rm bin/AntHybrid
