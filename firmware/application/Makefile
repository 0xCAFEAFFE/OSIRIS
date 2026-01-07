################################################################################
# OSIRIS Makefile
################################################################################

# Shell and tools
SHELL := cmd.exe
RM := del /Q

# Project name
PROJECT := OSIRIS_FW

# Directories
SRC_DIR := src
INC_DIR := inc
BUILD_DIR := build

# Source files
SRCS := \
	adc.c \
	cmd.c \
	gpio.c \
	keys.c \
	lcd.c \
	main.c \
	pwr.c \
	rad.c \
	rtc.c \
	sys.c \
	uart.c \
	ui.c

# Generate object and dependency file lists
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(SRCS:%.c=$(BUILD_DIR)/%.d)

# Output files
ELF_FILE := $(PROJECT).elf
HEX_FILE := $(PROJECT).hex
MAP_FILE := $(PROJECT).map

# Compiler flags
AVR_GCC_CFLAGS := \
	-x c \
	-funsigned-char \
	-funsigned-bitfields \
	-I"../shared" \
	-I"./$(INC_DIR)" \
	-Os \
	-Wall -Wextra -Wshadow \
	-Wdouble-promotion \
	-Wformat=2 \
	-Wundef \
	-fno-common \
	-flto \
	-ffunction-sections \
	-fdata-sections \
	-fshort-enums \
	-g2 \
	-mmcu=atmega328pb \
	-c \
	-std=gnu99 \
	-MD -MP

# Linker flags
LDFLAGS := \
	-Wl,-Map="$(MAP_FILE)" \
	-Wl,--start-group -Wl,-lm -Wl,--end-group \
	-Wl,--gc-sections \
	-mmcu=atmega328pb \
	-Wl,-u,vfprintf \
	-lprintf_flt \
	-lm

################################################################################
# Rules
################################################################################

# Default target
all: $(HEX_FILE) size

# Create build directory if it doesn't exist
$(BUILD_DIR):
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

# Pattern rule: compile .c files to .o files in build directory
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo.
	@echo Building file: $<
	avr-gcc $(AVR_GCC_CFLAGS) -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"

# Link object files to create ELF
$(ELF_FILE): $(OBJS)
	@echo.
	@echo Linking target: $@
	avr-gcc -o $@ $(OBJS) $(LDFLAGS)

# Convert ELF to HEX
$(HEX_FILE): $(ELF_FILE)
	@echo.
	@echo Creating HEX file: $@
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures "$<" "$@"

# Display size information
size: $(ELF_FILE)
	@echo.
	@echo Size information:
	avr-size "$(ELF_FILE)"
	avr-objdump -Pmem-usage "$(ELF_FILE)"

# Include dependency files
-include $(DEPS)

# Clean build artifacts
clean:
	@echo.
	@echo Cleaning build artifacts...
	-if exist $(BUILD_DIR) $(RM) $(BUILD_DIR)\*.o $(BUILD_DIR)\*.d
	-if exist $(BUILD_DIR) rmdir $(BUILD_DIR)
	-$(RM) $(ELF_FILE) $(HEX_FILE) $(MAP_FILE) 2>nul
	@echo.

# Phony targets
.PHONY: all clean size