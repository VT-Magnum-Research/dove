all: bin/acotreewidth

bin/acotreewidth: libaco liblocalsearch build/acotreewidth.o build/decomp.o
	cp ../../libaco/trunk/build/libaco.a lib/
	cp ../../liblocalsearch/trunk/build/liblocalsearch.a lib/
	g++ -static build/acotreewidth.o build/decomp.o -L./lib/ -laco -llocalsearch -o bin/acotreewidth -O3

build/acotreewidth.o: src/acotreewidth.cpp include/acotreewidth/decomp.h include/libaco/ants.h include/libaco/graph.h include/libaco/util.h include/liblocalsearch/localsearch.h
	g++ -Wall -ansi -I include/ -c src/acotreewidth.cpp -o build/acotreewidth.o -O3

build/decomp.o: src/decomp.cpp include/acotreewidth/decomp.h include/libaco/ants.h include/libaco/graph.h include/libaco/util.h include/liblocalsearch/localsearch.h
	g++ -Wall -ansi -I include/ -c src/decomp.cpp -o build/decomp.o -O3

include/libaco/%.h: ../../libaco/trunk/include/libaco/%.h
	cp ../../libaco/trunk/include/libaco/$*.h include/libaco/

include/liblocalsearch/%.h: ../../liblocalsearch/trunk/include/liblocalsearch/%.h
	cp ../../liblocalsearch/trunk/include/liblocalsearch/$*.h include/liblocalsearch/

libaco: .
	make -C ../../libaco/trunk -f ../../libaco/trunk/Makefile

liblocalsearch: .
	make -C ../../liblocalsearch/trunk -f ../../liblocalsearch/trunk/Makefile

clean: .
	rm include/libaco/*.h build/*.o bin/acotreewidth
