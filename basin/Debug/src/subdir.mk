# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/accept.c \
../src/block.c \
../src/collection.c \
../src/command.c \
../src/config.c \
../src/crafting.c \
../src/entity.c \
../src/game.c \
../src/inventory.c \
../src/item.c \
../src/json.c \
../src/log.c \
../src/main.c \
../src/nbt.c \
../src/network.c \
../src/packet.c \
../src/player.c \
../src/queue.c \
../src/smelting.c \
../src/streams.c \
../src/tileentity.c \
../src/tools.c \
../src/util.c \
../src/work.c \
../src/world.c \
../src/worldmanager.c \
../src/hashmap.c \
../src/profile.c \
../src/anticheat.c \
../src/plugin.c \
../src/xstring.c 

OBJS += \
./src/accept.o \
./src/block.o \
./src/collection.o \
./src/command.o \
./src/config.o \
./src/crafting.o \
./src/entity.o \
./src/game.o \
./src/inventory.o \
./src/item.o \
./src/json.o \
./src/log.o \
./src/main.o \
./src/nbt.o \
./src/network.o \
./src/packet.o \
./src/player.o \
./src/queue.o \
./src/smelting.o \
./src/streams.o \
./src/tileentity.o \
./src/tools.o \
./src/util.o \
./src/work.o \
./src/world.o \
./src/worldmanager.o \
./src/hashmap.o \
./src/profile.o \
./src/anticheat.o \
./src/plugin.o \
./src/xstring.o 

C_DEPS += \
./src/accept.d \
./src/block.d \
./src/collection.d \
./src/command.d \
./src/config.d \
./src/crafting.d \
./src/entity.d \
./src/game.d \
./src/inventory.d \
./src/item.d \
./src/json.d \
./src/log.d \
./src/main.d \
./src/nbt.d \
./src/network.d \
./src/packet.d \
./src/player.d \
./src/queue.d \
./src/smelting.d \
./src/streams.d \
./src/tileentity.d \
./src/tools.d \
./src/util.d \
./src/work.d \
./src/world.d \
./src/worldmanager.d \
./src/hashmap.d \
./src/profile.d \
./src/anticheat.d \
./src/plugin.d \
./src/xstring.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	$(CC) $(CFLAGS) -std=gnu11 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


