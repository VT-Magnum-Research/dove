EXE=generate_latency
SRC=$(wildcard *.cpp)
LATENCY_BIN=latency_impl/latency

CPP=g++
CFLAGS=-Wall -DLATENCY_BIN='"$(LATENCY_BIN)"'

all: $(EXE)

$(EXE): $(SRC) $(LATENCY_BIN); $(CPP) $(CFLAGS) $(SRC) -o $(EXE)
$(LATENCY_BIN): latency_impl/main.cpp;
	make -C latency_impl

clean: ; rm -f $(EXE)
