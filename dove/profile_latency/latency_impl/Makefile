
CFLAGS= -O3 $(shell mpicc --showme:compile)
LD_LIBRARY_PATH=/lib:/usr/lib64/openmpi

LDFLAGS=-L/lib $(shell mpicc --showme:link)
LIBS = $(LDFLAGS) -lmpi_cxx

# --- targets
all:
	g++ $(CFLAGS) $(LIBS) main.cpp -o latency
        
# --- remove binary and executable files
clean:
	rm -f latency


