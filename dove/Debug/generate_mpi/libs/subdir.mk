################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../generate_mpi/libs/graph.cpp 

OBJS += \
./generate_mpi/libs/graph.o 

CPP_DEPS += \
./generate_mpi/libs/graph.d 


# Each subdirectory must supply rules for building sources it contributes
generate_mpi/libs/%.o: ../generate_mpi/libs/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


