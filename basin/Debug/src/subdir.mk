################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/accept.c \
../src/collection.c \
../src/config.c \
../src/entity.c \
../src/inventory.c \
../src/log.c \
../src/main.c \
../src/nbt.c \
../src/network.c \
../src/packet1.8.c \
../src/packet1.9.c \
../src/player.c \
../src/queue.c \
../src/streams.c \
../src/tick.c \
../src/util.c \
../src/work.c \
../src/world.c \
../src/xstring.c 

OBJS += \
./src/accept.o \
./src/collection.o \
./src/config.o \
./src/entity.o \
./src/inventory.o \
./src/log.o \
./src/main.o \
./src/nbt.o \
./src/network.o \
./src/packet1.8.o \
./src/packet1.9.o \
./src/player.o \
./src/queue.o \
./src/streams.o \
./src/tick.o \
./src/util.o \
./src/work.o \
./src/world.o \
./src/xstring.o 

C_DEPS += \
./src/accept.d \
./src/collection.d \
./src/config.d \
./src/entity.d \
./src/inventory.d \
./src/log.d \
./src/main.d \
./src/nbt.d \
./src/network.d \
./src/packet1.8.d \
./src/packet1.9.d \
./src/player.d \
./src/queue.d \
./src/streams.d \
./src/tick.d \
./src/util.d \
./src/work.d \
./src/world.d \
./src/xstring.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


