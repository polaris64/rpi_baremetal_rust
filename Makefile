
CFLAGS = -mcpu=cortex-a7 -fpic -ffreestanding -std=gnu99 -O3 -Wall -Wextra

LDFLAGS = -T linker.ld -ffreestanding -nostdlib

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

RUST_TOOLCHAIN = arm-unknown-linux-gnueabi
#RUST_TOOLCHAIN = arm-unknown-linux-musleabi
RUST_LIB_DIR = target/$(RUST_TOOLCHAIN)/release/
RUST_LIB = $(RUST_LIB_DIR)libos_rpi.a

CARGO_PROFILE = --release

# ELF itermediate output
ELF = $(BUILD)output.elf

# patsubst matches whitespace-separated items from wildcard against the first
# argument (pattern) and replaces with the second argument. The % in the
# pattern matches all characters and is used in the replacement. Therefore, all
# source files are mapped to appropriate object files in the build directory.
AS_OBJECTS := $(patsubst $(SOURCE)%.S,$(BUILD)%.o,$(wildcard $(SOURCE)*.S))
C_OBJECTS  := $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))
RS_SOURCES := $(wildcard $(SOURCE)*.rs)

.PHONY: debug clean $(RUST_LIB)

all: $(TARGET) $(LIST)

debug: RUST_LIB_DIR = target/$(RUST_TOOLCHAIN)/debug/
debug: RUST_LIB = $(RUST_LIB_DIR)libos_rpi.a
debug: CFLAGS += -g -DDEBUG
debug: CARGO_PROFILE =
debug: $(TARGET) $(LIST)

# Generates the list file
$(LIST): $(ELF)
	$(TOOLCHAIN_PATH)$(ARMGNU)-objdump -d $(ELF) > $(LIST)

# Builds the target kernel image by copying the binary object code from
# the ELF executable
$(TARGET): $(ELF)
	$(TOOLCHAIN_PATH)$(ARMGNU)-objcopy $(ELF) -O binary $(TARGET)

# Links an ELF executable for the kernel from all assembled object
# files
$(ELF): $(AS_OBJECTS) $(C_OBJECTS) $(LINKER) $(RUST_LIB)
	echo $(RUST_LIB)
	$(TOOLCHAIN_PATH)$(ARMGNU)-gcc $(LDFLAGS) -o $(ELF) $(AS_OBJECTS) $(C_OBJECTS) -L$(RUST_LIB_DIR) -los_rpi

# Assembles all .S files in $SOURCE
$(BUILD)%.o: $(SOURCE)%.S $(BUILD)
	$(TOOLCHAIN_PATH)$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@

# Compiles all .c files in $SOURCE
$(BUILD)%.o: $(SOURCE)%.c $(BUILD)
	$(TOOLCHAIN_PATH)$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@

$(RUST_LIB):
	cargo build $(CARGO_PROFILE) --target $(RUST_TOOLCHAIN)

# Creates a build directory
$(BUILD):
	mkdir $@

clean:
	-rm -rf $(BUILD)
	-rm -f $(TARGET)
	-rm -f $(LIST)
	-rm -f $(MAP)
	cargo clean

run: $(ELF)
	qemu-system-arm -m 256 -M raspi2 -serial stdio -kernel $(ELF)

runbin: $(TARGET)
	qemu-system-arm -m 256 -M raspi2 -serial stdio -kernel $(TARGET)

dbg: debug $(ELF)
	$(TOOLCHAIN_PATH)$(ARMGNU)-gdb $(ELF)

dbgrun: debug $(ELF) gdbinit
	qemu-system-arm -m 256 -no-reboot -M raspi2 -serial stdio -kernel $(ELF) -S -s

dbgrunbin: debug $(TARGET) gdbinit
	qemu-system-arm -m 256 -no-reboot -M raspi2 -serial stdio -kernel $(TARGET) -S -s

gdbinit:
	echo "add-symbol-file $(ELF) 0x8000" > .gdbinit
	echo "target remote localhost:1234" >> .gdbinit
	echo "break kernel_main" >> .gdbinit
