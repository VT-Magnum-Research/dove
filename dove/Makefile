CFLAGS := -g 
INC    :=-Ilibs/rapidxml-1.13
 
all: 
	$(CXX) -c $(CFLAGS) $(INC) -o dove.o dove.cpp
	ar rvs libdove.a dove.o
	ranlib libdove.a

# --- remove binary and executable files
clean:
	rm -f dove.o libdove.a


