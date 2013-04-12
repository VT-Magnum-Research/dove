

DOVE_ROOT    ?= $(CURDIR)/../../dove
LIBS         := -L$(DOVE_ROOT) -ldove
INC          := -I$(DOVE_ROOT) -Isrc/utils
CXXFLAGS     += -g -Wl,--allow-multiple-definition
# TODO: I don't know if it's ever a good idea to pass the
# --allow-multiple-definiton option, but without it, there are
# "multiple definition conflicts" between TCLAPP within the
# dove library, and TCLAPP within this.
#
# This isn't ideal, and I don't like making people that use dove use this,
# but the other solution of this I'm aware
# of is to modify the `dove` object file and localize the
# conflicting symbols.
#
# There could be another way, or
# I could be misinterpreting this in general.
#
# @Hamilton, thoughts?

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
