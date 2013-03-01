
CFLAGS= -O3 -I/usr/include/openmpi-x86_64
LD_LIBRARY_PATH=/usr/local/lib:/usr/lib64/openmpi/lib

LDFLAGS=-L/usr/local/lib -L/usr/lib64/openmpi/lib
LIBS = $(LDFLAGS) -lmpi -lmpi_cxx 

# --- targets
all:
	g++ $(CFLAGS) $(LIBS) main.cpp -o latency
        
# --- remove binary and executable files
clean:
	rm -f latency


