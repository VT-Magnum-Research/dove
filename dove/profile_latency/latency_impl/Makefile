
CFLAGS= -O3 $(shell mpicc --showme:compile)
LD_LIBRARY_PATH=/usr/local/lib:/usr/lib64/openmpi/lib

LDFLAGS=-L/usr/local/lib $(shell mpicc --showme:link)
LIBS = $(LDFLAGS) -lmpi_cxx

# --- targets
all:
	g++ $(CFLAGS) $(LIBS) main.cpp -o latency
        
# --- remove binary and executable files
clean:
	rm -f latency


