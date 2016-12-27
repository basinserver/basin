################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/accept.c \
../src/block.c \
../src/collection.c \
../src/config.c \
../src/craft.c \
../src/entity.c \
../src/game.c \
../src/inventory.c \
../src/json.c \
../src/log.c \
../src/main.c \
../src/nbt.c \
../src/network.c \
../src/packet.c \
../src/player.c \
../src/queue.c \
../src/streams.c \
../src/util.c \
../src/work.c \
../src/world.c \
../src/worldmanager.c \
../src/xstring.c 

OBJS += \
./src/accept.o \
./src/block.o \
./src/collection.o \
./src/config.o \
./src/craft.o \
./src/entity.o \
./src/game.o \
./src/inventory.o \
./src/json.o \
./src/log.o \
./src/main.o \
./src/nbt.o \
./src/network.o \
./src/packet.o \
./src/player.o \
./src/queue.o \
./src/streams.o \
./src/util.o \
./src/work.o \
./src/world.o \
./src/worldmanager.o \
./src/xstring.o 

C_DEPS += \
./src/accept.d \
./src/block.d \
./src/collection.d \
./src/config.d \
./src/craft.d \
./src/entity.d \
./src/game.d \
./src/inventory.d \
./src/json.d \
./src/log.d \
./src/main.d \
./src/nbt.d \
./src/network.d \
./src/packet.d \
./src/player.d \
./src/queue.d \
./src/streams.d \
./src/util.d \
./src/work.d \
./src/world.d \
./src/worldmanager.d \
./src/xstring.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu11 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


