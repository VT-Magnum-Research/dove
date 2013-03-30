
# Setup some sensible defaults
DOVE_ROOT ?= $(CURDIR)/..
XML_ROOT  ?= $(CURDIR)/../libs/rapidxml-1.13

CPP ?= gcc
EXE=generate_latency
SRC=$(wildcard *.cpp)
LATENCY_BIN=latency_impl/latency

INC = -I$(DOVE_ROOT) -I$(XML_ROOT)

all: $(EXE)

$(EXE): $(SRC) $(LATENCY_BIN); 
	$(CPP) $(CFLAGS) -DLATENCY_BIN='"$(LATENCY_BIN)"' $(INC) $(SRC) -o $(EXE)

$(LATENCY_BIN): latency_impl/main.cpp;
	cd latency_impl && $(MAKE)

clean: ; rm -f $(EXE)
