CFLAGS= -g -ggdb $(shell mpicc --showme:compile)
LD_LIBRARY_PATH=/usr/local/lib:/usr/lib64/openmpi/lib
LDFLAGS=-L/lib $(shell mpicc --showme:link)
LIBS = $(LDFLAGS) -lmpi_cxx -lboost_serialization -lboost_mpi 

# Terminology: 
# model = input model e.g. STL file
# generator = code generator
# impl = MPI-based executable generated from STL model

all: main.cpp libs/graph.cpp
	g++ $(CFLAGS) $(LIBS) libs/graph.cpp main.cpp -o bin/generator
       
# --- remove binary and executable files
clean:
	rm -f bin/generator 


