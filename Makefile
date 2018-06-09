# Toolchain path
TOOLCHAIN_PATH = toolchain/bin/

# Default toolchain to use
ARMGNU ?= arm-none-eabi

# Build directory
BUILD = build/

# Source directory
SOURCE = src/

# Output file name
TARGET = kernel.img

# Assembler listing
LIST = kernel.list

# Map file
MAP = kernel.map

# Name of the linker script to use
LINKER = linker.ld

# ELF itermediate output
ELF = $(BUILD)output.elf

# patsubst builds a list of objects from $SOURCE/*.S (last arg)
# converting [NAME].S to [NAME].o
AS_OBJECTS := $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S))
C_OBJECTS  := $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))

all: $(TARGET) $(LIST)

# Generates the list file
$(LIST): $(ELF)
	$(TOOLCHAIN_PATH)$(ARMGNU)-objdump -d $(ELF) > $(LIST)

# Builds the target kernel image by copying the binary object code from
# the ELF executable
$(TARGET): $(ELF)
	$(TOOLCHAIN_PATH)$(ARMGNU)-objcopy $(ELF) -O binary $(TARGET)

# Links an ELF executable for the kernel from all assembled object
# files
$(ELF): $(AS_OBJECTS) $(C_OBJECTS) $(LINKER)
	$(TOOLCHAIN_PATH)$(ARMGNU)-gcc -T linker.ld -o $(ELF) -ffreestanding -nostdlib -Wl,-Map,$(MAP) $(AS_OBJECTS) $(C_OBJECTS)

# Assembles all .S files in $SOURCE
$(BUILD)%.o: $(SOURCE)%.S $(BUILD)
	$(TOOLCHAIN_PATH)$(ARMGNU)-gcc -mcpu=cortex-a7 -fpic -ffreestanding -c $< -o $@
	#$(TOOLCHAIN_PATH)$(ARMGNU)-as -I $(SOURCE) $< -o $@

# Compiles all .c files in $SOURCE
$(BUILD)%.o: $(SOURCE)%.c $(BUILD)
	$(TOOLCHAIN_PATH)$(ARMGNU)-gcc -mcpu=cortex-a7 -fpic -ffreestanding -std=gnu99 -O0 -Wall -Wextra -c $< -o $@ 

# Creates a build directory
$(BUILD):
	mkdir $@

clean:
	-rm -rf $(BUILD)
	-rm -f $(TARGET)
	-rm -f $(LIST)
	-rm -f $(MAP)
