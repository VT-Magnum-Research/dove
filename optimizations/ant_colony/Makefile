

DOVE_ROOT    ?= $(CURDIR)/../../dove
LIBS         := -L$(DOVE_ROOT) -ldove
INC          := -I$(DOVE_ROOT) -Isrc/utils
CXXFLAGS     += -g 

all: bin/AntHybrid

# The following variables and rules make 
# the utils/ folder automatically
#
util_cpp := $(wildcard src/utils/*.cpp)
util_o   := $(addprefix build/,$(notdir $(util_cpp:.cpp=.o)))
build/%.o: src/utils/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

bin/AntHybrid: $(util_o)
	$(CXX) $(CXXFLAGS) $(INC) -c -o build/mps.o src/mps.cpp 
	$(CXX) $(CXXFLAGS) $(INC) -c -o build/acomps.o src/acomps.cpp
	$(CXX) $(CXXFLAGS) -o bin/AntHybrid build/*.o $(LIBS)

clean: 
	rm -f build/*.o
	rm -f bin/AntHybrid
