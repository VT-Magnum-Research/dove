
# Setup some sensible defaults
DOVE_ROOT ?= $(CURDIR)/..
XML_ROOT  ?= $(CURDIR)/../libs/rapidxml-1.13
INC       := -I$(DOVE_ROOT) -I$(XML_ROOT)
LIBS      := -L$(DOVE_ROOT) -ldove 
CXXFLAGS  += -g -std=c++0x

EXE=generate_latency
LATENCY_BIN=latency_impl/latency

SRC=$(wildcard *.cpp)
all: $(EXE)

$(EXE): $(SRC) $(LATENCY_BIN); 
	$(CXX) $(CXXFLAGS) $(INC) -DLATENCY_BIN='"$(LATENCY_BIN)"' $(SRC) -o $(EXE) $(LIBS)

$(LATENCY_BIN): latency_impl/main.cpp;
	cd latency_impl && $(MAKE)

clean: ; rm -f $(EXE)
