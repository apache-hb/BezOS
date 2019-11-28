CC32 = i386-elf-gcc
ASM32 = i386-elf-as
BUILD_DIR = Build
SRC_DIR = Source

# x86:
# 	mkdir -p $(BUILD_DIR)
# 	$(ASM32) Source/Boot.s -o $(BUILD_DIR)/Boot.o
# 	$(CC32) -c Source/Kernel.c -o $(BUILD_DIR)/Kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
# 	$(CC32) -T Source/Linker.ld -o $(BUILD_DIR)/BezOS.bin -ffreestanding -O2 -nostdlib $(BUILD_DIR)/Boot.o $(BUILD_DIR)/Kernel.o -lgcc

# CC64 = x86_64-apple-darwin18.7.0-gcc-7
# ASM64 = x86_64-apple-darwin18.7.0-gcc-ar-7

all:
	mkdir -p $(BUILD_DIR)
	$(ASM) Source/Boot.o -o $(BUILD_DIR)/Boot.o
	$(CC) -m64 -c $(SRC_DIR)/Kernel.c -o $(BUILD_DIR)/Kernel.o -ffreestanding -z max-page-size=0x1000 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -std=gnu99 -O2 -Wall -Wextra
	$(CC) -T $(SRC_DIR)/Linker.ld