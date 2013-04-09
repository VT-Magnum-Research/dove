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
export RUNNER_ROOT  := $(DOVE_ROOT)/runner
export XML_ROOT     := $(DOVE_ROOT)/libs/rapidxml_1.13
export TCLAP_ROOT   := $(DOVE_ROOT)/libs/tclap
export LIBDOVE      := $(DOVE_ROOT)/libdove.a

# Dove core requires C++11
export CFLAGS=-std=c++0x

custom: all aco
	# Perform some cleaning 
	rm -f ~/dove-diamond/*
	# Create working directory for dove
	mkdir -p ~/dove-diamond
	# Skip profiling by using a saved profile
	cp ~/cores/ataack.xml.complete ~/dove-diamond/system.xml
	# Choose a software model
	cp stg-samples/diamond.stg ~/dove-diamond/software.stg
	# Run the optimization.stg
	cd optimizations/ant_colony && ./bin/AntHybrid -d ~/dove-diamond/ --simple -c 12
	# Run the generator on newly-created deployments.xml
	cd $(GEN_ROOT) && ./bin/generator --debug --genhosts -r -o ~/dove-diamond/ -y ~/dove-diamond/system.xml -d ~/dove-diamond/deployments.xml -s ~/dove-diamond/software.stg
	# Build the executable from the generator code
	cd ~/dove-diamond && $(MAKE)
	# Run the (sample) runner on the newly-created rankfiles
	#cd ~/dove2 && sh runmpi.sh
	cd $(RUNNER_ROOT) && ./runner -m 1 -k 1 -l -d ~/dove-diamond/

custom50: all aco 
	rm -f ~/dove-50/*
	mkdir -p ~/dove-50
	cp ~/cores/ataack.xml.complete ~/dove-50/system.xml
	cp ~/Qualifier/STG-benchmarks/50/rand0000.stg ~/dove-50/software.stg
	cd optimizations/ant_colony && ./bin/AntHybrid -d ~/dove-50/ --simple -c 12
	cd $(GEN_ROOT) && ./bin/generator --genhosts -r -o ~/dove-50/ -y ~/dove-50/system.xml -d ~/dove-50/deployments.xml -s ~/dove-50/software.stg
	cd ~/dove-50 && $(MAKE)
	cd $(RUNNER_ROOT) && ./runner -l -d ~/dove-50/

custom300: all aco 
	rm -f ~/dove-300/*
	mkdir -p ~/dove-300
	cp ~/cores/ataack.xml.complete ~/dove-300/system.xml
	cp ~/Qualifier/STG-benchmarks/300/rand0000.stg ~/dove-300/software.stg
	cd optimizations/ant_colony && ./bin/AntHybrid -d ~/dove-300/ --simple -c 12
	cd $(GEN_ROOT) && ./bin/generator --genhosts -r -o ~/dove-300/ -y ~/dove-300/system.xml -d ~/dove-300/deployments.xml -s ~/dove-300/software.stg
	cd ~/dove-300 && $(MAKE)
	cd $(RUNNER_ROOT) && ./runner -l -d ~/dove-300/

custom1000: all aco 
	rm -f ~/dove-1000/*
	mkdir -p ~/dove-1000
	cp ~/cores/ataack.xml.complete ~/dove-1000/system.xml
	cp ~/Qualifier/STG-benchmarks/1000/rand0000.stg ~/dove-1000/software.stg
	cd optimizations/ant_colony && ./bin/AntHybrid -d ~/dove-1000/ --simple -c 12
	cd $(GEN_ROOT) && ./bin/generator --genhosts -r -o ~/dove-1000/ -y ~/dove-1000/system.xml -d ~/dove-1000/deployments.xml -s ~/dove-1000/software.stg
	cd ~/dove-1000 && $(MAKE)
	cd $(RUNNER_ROOT) && ./runner -l -d ~/dove-1000/

custom10002: all aco 
	rm -f ~/dove-10002/*
	mkdir -p ~/dove-10002
	cp ~/cores/ataack.xml.complete ~/dove-10002/system.xml
	cp ~/Qualifier/STG-benchmarks/10002/rand0000.stg ~/dove-10002/software.stg
	cd optimizations/ant_colony && ./bin/AntHybrid -d ~/dove-10002/ --simple -c 12
	cd $(GEN_ROOT) && ./bin/generator --genhosts -r -o ~/dove-10002/ -y ~/dove-10002/system.xml -d ~/dove-10002/deployments.xml -s ~/dove-10002/software.stg
	cd ~/dove-10002 && $(MAKE)
	cd $(RUNNER_ROOT) && ./runner -m 100 -l -d ~/dove-10002/

all: libdove.a runner generator latency

# DOVE targets
libdove.a:
	cd $(DOVE_ROOT) && $(MAKE)

generator:
	cd $(GEN_ROOT) && $(MAKE)

latency:
	cd $(PROFILE_ROOT) && $(MAKE)

runner:
	cd $(RUNNER_ROOT) && $(MAKE)

# Optimization targets
saaco: libdove.a
	cd optimizations/saaco && $(MAKE)

aco: libdove.a
	cd optimizations/ant_colony && $(MAKE)

# --- remove binary and executable files
.PHONY: cleanall cleancore
cleanall: cleancore
	cd optimizations/ant_colony && $(MAKE) clean
	cd optimizations/saaco      && $(MAKE) clean

cleancore: 
	cd $(RUNNER_ROOT)           && $(MAKE) clean
	cd $(PROFILE_ROOT)          && $(MAKE) clean
	cd $(GEN_ROOT)              && $(MAKE) clean
	cd $(DOVE_ROOT)             && $(MAKE) clean


