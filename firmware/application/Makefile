################################################################################
# Cross-Platform Makefile for OSIRIS
################################################################################

# Detect OS
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

# Platform-specific settings
ifeq ($(DETECTED_OS),Windows)
    SHELL := cmd.exe
    RM := del /Q
    RMDIR := rmdir /S /Q
    MKDIR := mkdir
    PATHSEP := \\
    DEVNULL := 2>nul
    FIXPATH = $(subst /,\,$1)
    ECHO_BLANK := @echo.
else
    RM := rm -f
    RMDIR := rm -rf
    MKDIR := mkdir -p
    PATHSEP := /
    DEVNULL := 2>/dev/null
    FIXPATH = $1
    ECHO_BLANK := @echo
endif

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
CFLAGS := \
	-x c \
	-funsigned-char \
	-funsigned-bitfields \
	-DDEBUG \
	-I../shared \
	-I./$(INC_DIR) \
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
	-Wl,-Map=$(MAP_FILE) \
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
all: $(HEX_FILE)
	$(ECHO_BLANK)
	@echo Build complete!
	$(ECHO_BLANK)
	@avr-objdump -Pmem-usage $(ELF_FILE)

# Create build directory if it doesn't exist
$(BUILD_DIR):
ifeq ($(DETECTED_OS),Windows)
	@if not exist $(call FIXPATH,$(BUILD_DIR)) $(MKDIR) $(call FIXPATH,$(BUILD_DIR))
else
	@$(MKDIR) $(BUILD_DIR)
endif

# Pattern rule: compile .c files to .o files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo Building: $<
	@avr-gcc $(CFLAGS) -MF $(call FIXPATH,$(@:%.o=%.d)) -MT $(call FIXPATH,$@) -o $(call FIXPATH,$@) $(call FIXPATH,$<)

# Link object files to create ELF
$(ELF_FILE): $(OBJS)
	@echo Linking: $@
	@avr-gcc -o $@ $(OBJS) $(LDFLAGS)

# Convert ELF to HEX
$(HEX_FILE): $(ELF_FILE)
	@echo Creating: $@
	@avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures $< $@

# Include dependency files (automatic header dependency tracking)
-include $(DEPS)

# Clean build artifacts
clean:
	@echo Cleaning...
ifeq ($(DETECTED_OS),Windows)
	@if exist $(call FIXPATH,$(BUILD_DIR))\*.o $(RM) $(call FIXPATH,$(BUILD_DIR)\*.o) $(DEVNULL)
	@if exist $(call FIXPATH,$(BUILD_DIR))\*.d $(RM) $(call FIXPATH,$(BUILD_DIR)\*.d) $(DEVNULL)
	@if exist $(call FIXPATH,$(BUILD_DIR)) $(RMDIR) $(call FIXPATH,$(BUILD_DIR)) $(DEVNULL)
	@$(RM) $(ELF_FILE) $(HEX_FILE) $(MAP_FILE) $(DEVNULL)
	@$(RM) $(PROJECT).a $(PROJECT).eep $(PROJECT).lss $(PROJECT).srec $(PROJECT).usersignatures $(DEVNULL)
else
	@$(RM) $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d $(DEVNULL)
	@$(RMDIR) $(BUILD_DIR) $(DEVNULL)
	@$(RM) $(ELF_FILE) $(HEX_FILE) $(MAP_FILE) $(DEVNULL)
	@$(RM) $(PROJECT).a $(PROJECT).eep $(PROJECT).lss $(PROJECT).srec $(PROJECT).usersignatures $(DEVNULL)
endif
	@echo Clean complete.

# Phony targets
.PHONY: all clean