################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/dim/CommandConnector.cpp \
../src/dim/MonitorConnector.cpp 

OBJS += \
./src/dim/CommandConnector.o \
./src/dim/MonitorConnector.o 

CPP_DEPS += \
./src/dim/CommandConnector.d \
./src/dim/MonitorConnector.d 


# Each subdirectory must supply rules for building sources it contributes
src/dim/%.o: ../src/dim/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -pipe -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

