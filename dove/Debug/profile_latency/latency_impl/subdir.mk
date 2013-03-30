################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../profile_latency/latency_impl/main.cpp 

OBJS += \
./profile_latency/latency_impl/main.o 

CPP_DEPS += \
./profile_latency/latency_impl/main.d 


# Each subdirectory must supply rules for building sources it contributes
profile_latency/latency_impl/%.o: ../profile_latency/latency_impl/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


