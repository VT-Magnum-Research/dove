CXX=mpic++
CFLAGS= -g -ggdb

# Terminology: 
# model = input model e.g. STL file
# generator = code generator
# impl = MPI-based executable generated from STL model

all: main.cpp libs/graph.cpp
	$(CXX) $(CFLAGS) libs/graph.cpp main.cpp -o bin/generator
       
clean:
	rm -f bin/generator 


