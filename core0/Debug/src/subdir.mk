################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

CPP_SRCS += \
../src/CpuMailbox.cpp \
../src/CpuMessage.cpp \
../src/Helpers.cpp \
../src/HmiController.cpp \
../src/HwClock.cpp \
../src/InterruptManager.cpp \
../src/LiteQueue.cpp \
../src/Main.cpp \
../src/Middleware.cpp \
../src/Platform.cpp \
../src/PowerController.cpp \
../src/Terminal.cpp 

OBJS += \
./src/CpuMailbox.o \
./src/CpuMessage.o \
./src/Helpers.o \
./src/HmiController.o \
./src/HwClock.o \
./src/InterruptManager.o \
./src/LiteQueue.o \
./src/Main.o \
./src/Middleware.o \
./src/Platform.o \
./src/PowerController.o \
./src/Terminal.o 

CPP_DEPS += \
./src/CpuMailbox.d \
./src/CpuMessage.d \
./src/Helpers.d \
./src/HmiController.d \
./src/HwClock.d \
./src/InterruptManager.d \
./src/LiteQueue.d \
./src/Main.d \
./src/Middleware.d \
./src/Platform.d \
./src/PowerController.d \
./src/Terminal.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 g++ compiler'
	arm-none-eabi-g++ -DMULTIIO_DEBUG -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard --std=c++20 -I/home/jacobpsimos/MultiIO/Vitis/Workspace/multiio_platform/export/multiio_platform/sw/multiio_platform/cortexa9_0_domain/bspinclude/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


