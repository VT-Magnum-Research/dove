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


# --- remove binary and executable files
clean:
	cd optimizations/saaco && $(MAKE) clean


