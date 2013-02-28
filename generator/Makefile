# Usage 
#  $ export STG="path to STG file"
#  $ make 

CFLAGS=  -O3
LIBS = -L/usr/local/lib -lmpi -lmpi_cxx -lboost_serialization -lboost_mpi -lm 
IMPLCPP=impl.cpp
IMPLO=impl

# Terminology: 
# model = input model e.g. STL file
# generator = code generator
# impl = MPI-based executable generated from STL model

# --- targets
all: generator impl
impl: 
	g++ $(CFLAGS) $(LIBS) gen/$(IMPLCPP) -o bin/$(IMPLO)
        
generator: graph.cpp graph.h main.cpp main.h
	g++ $(CFLAGS) $(LIBS) graph.cpp main.cpp -o bin/generator
	./bin/generator "$(STG)" gen/$(IMPLCPP)
       
# --- remove binary and executable files
clean:
	rm -f bin/generator gen/$(IMPLCPP) bin/$(IMPLO)


