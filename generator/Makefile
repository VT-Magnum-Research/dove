# Usage 
#  $ export STG="path to STG file"
#  $ make 

IMPLCPP=impl.cpp
IMPLO=impl

CFLAGS= -g -ggdb -I/usr/include/openmpi-x86_64
LD_LIBRARY_PATH=/usr/local/lib:/usr/lib64/openmpi/lib
LDFLAGS=-L/usr/local/lib -L/usr/lib64/openmpi/lib
LIBS = $(LDFLAGS) -lmpi -lmpi_cxx -lboost_serialization -lboost_mpi 


# Terminology: 
# model = input model e.g. STL file
# generator = code generator
# impl = MPI-based executable generated from STL model

# --- targets
all: generator impl
impl: 
	g++ $(CFLAGS) $(LIBS) gen/$(IMPLCPP) -o bin/$(IMPLO)
        
generator: libs/graph.cpp libs/graph.h main.cpp main.h
	g++ $(CFLAGS) $(LIBS) libs/graph.cpp main.cpp -o bin/generator
	./bin/generator "$(STG)" gen/$(IMPLCPP)
       
# --- remove binary and executable files
clean:
	rm -f bin/generator gen/$(IMPLCPP) bin/$(IMPLO)


