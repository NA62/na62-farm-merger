################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/main.cpp 

OBJS += \
./src/main.o 

CPP_DEPS += \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -I/afs/cern.ch/sw/IntelSoftware/linux/x86_64/xe2013/composer_xe_2013_sp1.2.144/ipp/include -I/afs/cern.ch/sw/IntelSoftware/linux/x86_64/xe2013/composer_xe_2013_sp1.2.144/mkl/include -I/afs/cern.ch/sw/IntelSoftware/linux/x86_64/xe2013/composer_xe_2013_sp1.2.144/tbb/include -O3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

