#!/bin/sh

# Build hosts file
read -d '' String <<"EOF"
# This is an auto-generated hostfile 
# 
# By not setting max-slots we trigger MPI to 
# run in oversubscribed mode

# The following is a quad-core machine
Hamiltons-MacBook-Pro.local slots=4

EOF

echo "${String}"

echo "${String}" > hosts

# mpirun -display-allocation -display-map -report-bindings -np 52 --hostfile hosts
