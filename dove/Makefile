DEBUG=-g
CFLAGS=$(DEBUG) 
INC=-Ilibs/rapidxml-1.13
SRC=dove.cpp dove_xml.cpp
OBJ=$(SRC:.cpp=.o)
 
all: libdove.a

libdove.a: $(OBJ)
	ar rvs $@ $(OBJ)
	ranlib $@

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $(INC) $^

.PHONY: clean
clean:
	rm -f $(OBJ) libdove.a
