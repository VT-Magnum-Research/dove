all: bin/acotemplate

bin/acotemplate: libaco liblocalsearch build/acotemplate.o build/template.o
	cp ../../libaco/trunk/build/libaco.a lib/
	cp ../../liblocalsearch/trunk/build/liblocalsearch.a lib/
	g++ -static build/acotemplate.o build/template.o -L./lib/ -laco -o bin/acotemplate

build/acotemplate.o: src/acotemplate.cpp include/acotemplate/template.h include/libaco/ants.h include/libaco/graph.h include/libaco/util.h
	g++ -ansi -I include/ -c src/acotemplate.cpp -o build/acotemplate.o

build/template.o: src/template.cpp include/libaco/ants.h include/libaco/graph.h include/libaco/util.h include/liblocalsearch/localsearch.h
	g++ -ansi -I include/ -c src/template.cpp -o build/template.o

include/libaco/%.h: ../../libaco/trunk/include/libaco/%.h
	cp ../../libaco/trunk/include/libaco/$*.h include/libaco/

include/liblocalsearch/%.h: ../../liblocalsearch/trunk/include/liblocalsearch/%.h
	cp ../../liblocalsearch/trunk/include/liblocalsearch/$*.h include/liblocalsearch/

libaco:
	make -C ../../libaco/trunk -f ../../libaco/trunk/Makefile

liblocalsearch:
	make -C ../../liblocalsearch/trunk -f ../../liblocalsearch/trunk/Makefile

clean: .
	rm include/libaco/*.h build/*.o bin/acotemplate
