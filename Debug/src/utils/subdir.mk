################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/utils/AExecutable.cpp \
../src/utils/Stopwatch.cpp \
../src/utils/Utils.cpp 

OBJS += \
./src/utils/AExecutable.o \
./src/utils/Stopwatch.o \
./src/utils/Utils.o 

CPP_DEPS += \
./src/utils/AExecutable.d \
./src/utils/Stopwatch.d \
./src/utils/Utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/utils/%.o: ../src/utils/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -pipe -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


