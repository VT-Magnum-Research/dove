


CFLAGS=-c -std=c++0x
LIBS =-Ilibs/rapidxml-1.13/
 
all: 
	g++ $(CFLAGS) $(LIBS) -o dove.o dove.hpp
	ar rcs libdove.a dove.o

# --- remove binary and executable files
clean:
	rm -f dove.o libdove.a


