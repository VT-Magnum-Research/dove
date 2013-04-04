# --------- README --------
#  This Makefile sets up defaults for any sub makefiles, such 
#  as variables pointing to the root of different parts of the 
#  Deployment Optimization Validation Engine (DOVE) source tree. 
#  Sub makefiles should consider the following rules: In general, 
#  never override CFLAGS, only append e.g. CFLAGS+=<your flags>. 
#  To support make-ing from your makefile directly, be sure to set
#  defaults for the variables you need e.g. DOVE_ROOT?=<path from
#  your makefile>
#  ------------------------

# TODO define all optimizations as a variable and 
# create targets based off of directories in the 
# optimizations directory

export DOVE_ROOT    := $(CURDIR)/dove
export GEN_ROOT     := $(DOVE_ROOT)/generate_mpi
export PROFILE_ROOT := $(DOVE_ROOT)/profile_latency
export XML_ROOT     := $(DOVE_ROOT)/libs/rapidxml_1.13
export TCLAP_ROOT   := $(DOVE_ROOT)/libs/tclap
export LIBDOVE      := $(DOVE_ROOT)/libdove.a

# Dove core requires C++11
export CFLAGS=-std=c++0x

custom: cust_clean libdove.a aco
	# Create working directory for dove
	mkdir -p ~/dove2
	# Skip profiling by using a saved profile
	cp ~/cores/ataack.xml.complete ~/dove2/system.xml
	# Choose a software model
	cp stg-samples/diamond.stg ~/dove2/software.stg
	# Run the optimization
	cd optimizations/ant_colony && ./bin/AntHybrid -d ~/dove2/ --simple -c 2
	# Run the generator on newly-created deployments.xml
	cd dove/generate_mpi && ./bin/generator --genhosts -r -o ~/dove2/ -y ~/dove2/system.xml -d ~/dove2/deployments.xml -s ~/dove2/software.stg
	# Build the executable from the generator code
	cd ~/dove2 && $(MAKE)
	# Run the (sample) runner on the newly-created rankfiles
	cd ~/dove2 && sh runmpi.sh
	# runner --rmrankfiles -l -d ~/dove2/

cust_clean:
	cd $(DOVE_ROOT) && $(MAKE) clean
	cd optimizations/ant_colony && $(MAKE) clean
	rm -f ~/dove2/*

all: generator latency libdove.a

# DOVE targets
libdove.a:
	cd $(DOVE_ROOT) && $(MAKE)

generator:
	cd $(GEN_ROOT) && $(MAKE)

latency:
	cd $(PROFILE_ROOT) && $(MAKE)

# Optimization targets
saaco: libdove.a
	cd optimizations/saaco && $(MAKE)

aco: libdove.a
	cd optimizations/ant_colony && $(MAKE)

# --- remove binary and executable files
clean:
	cd optimizations/saaco && $(MAKE) clean


