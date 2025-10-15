# Version
MAJOR = 1
MINOR = 0
RELEASE = 0

# LLVM Toolchain
LLVM_CC = clang
LLVM_OPT = opt
LLC = llc
LLVM_MC = llvm-mc
LLVM_LD = clang
LLVM_AR = llvm-ar

# Flags
CFLAGS = -I/usr/local/include 
CFLAGS += -Wall -Wextra -Wpedantic -Wshadow -Wconversion -g -Iinclude -Wno-gnu-zero-variadic-macro-arguments -O2
ASM_FLAGS = $(CFLAGS)
OPT_FLAGS = -O2 -strip-debug
LD_FLAGS = 
LD_LIB += -lc -lpthread -lxcserial -lxcxml

# Source and build directories
SRC_DIR = src
BUILD_DIR = build
LLVM_IR_DIR = $(BUILD_DIR)/llvm_ir
ASM_DIR = $(BUILD_DIR)/asm

# Documentation
DOCS_DIR = docs

# --- Target 1: mixlink ---
TARGET_NAME = mixlink
BIN = $(BUILD_DIR)/$(TARGET_NAME)

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
LL_FILES = $(patsubst $(SRC_DIR)/%.c, $(LLVM_IR_DIR)/%.ll, $(SRCS))
OPT_LL_FILES = $(patsubst $(SRC_DIR)/%.c, $(LLVM_IR_DIR)/%.opt.ll, $(SRCS))
ASM_FILES = $(patsubst $(SRC_DIR)/%.c, $(ASM_DIR)/%.s, $(SRCS))

# --- Documentation arguments ---
PROJECT_NAME = $(TARGET_NAME)
PROJECT_NAME_BRIEF = "A C serial port library for Linux (lib$(PROJECT_NAME))"
PROJECT_BRIEF = "$(PROJECT_NAME) is a lightweight C wrapper library for interfacing with non-canonical serial port devices. Supports synchronous and asynchronous communication, as well as automatic hotplug detection and reconnection."

# --- Build Rules ---
single:
	@echo "Creating the executable for the host platform..."
	$(MAKE) clean
	$(MAKE) $(BIN) TARGET_ARCH_LLC="" TARGET_ARCH_CC="" TYPE=so CF=-fPIC LF="-relocation-model=pic" LDF="" TARGET_INC=""

all: 
	@echo "Creating the libraries for the following platforms:"
	@echo "aarch64, x86-64, arm"
	$(MAKE) clean
	$(MAKE) release TARGET_ARCH_LLC=arm TARGET_ARCH_CC=arm-linux-gnueabihf TYPE=so CF=-fPIC LF="-relocation-model=pic" LDF="" TARGET_INC="--target="
	$(MAKE) clean
	$(MAKE) release TARGET_ARCH_LLC=x86-64 TARGET_ARCH_CC=x86_64-linux-gnu TYPE=so CF=-fPIC LF="-relocation-model=pic" LDF="" TARGET_INC="--target="
	$(MAKE) clean
	$(MAKE) release TARGET_ARCH_LLC=aarch64 TARGET_ARCH_CC=aarch64-linux-gnu TYPE=so CF+=-fPIC LF="-relocation-model=pic" LDF="" TARGET_INC="--target="

# Create build directories
$(BUILD_DIR) $(LLVM_IR_DIR) $(ASM_DIR):
	@mkdir -p $@
	@echo "Created directory $@"

# Compile .c or .cpp to LLVM IR (.ll)
$(LLVM_IR_DIR)/%.ll: $(SRC_DIR)/%.c | $(LLVM_IR_DIR)
	@echo "Compiling $< to LLVM IR $@"
	$(LLVM_CC) -std=gnu99 $(CFLAGS) $(CF) -S -emit-llvm $(TARGET_INC)$(TARGET_ARCH_CC) $< -o $@

# Optimize LLVM IR (optional)
$(LLVM_IR_DIR)/%.opt.ll: $(LLVM_IR_DIR)/%.ll
	@echo "Optimizing LLVM IR $< to $@"
	$(LLVM_OPT) $(OPT_FLAGS) $< -o $@

# Compile LLVM IR (.ll) to Assembly (.s)
$(ASM_DIR)/%.s: $(LLVM_IR_DIR)/%.opt.ll | $(ASM_DIR)
	@echo "Compiling LLVM IR $< to Assembly $@"
	$(LLC) -march=$(TARGET_ARCH_LLC) $(LF) $< -o $@

# Assemble Assembly (.s) to Object (.o)
$(BUILD_DIR)/%.o: $(ASM_DIR)/%.s | $(BUILD_DIR)
	@echo "Assembling $< to Object $@"
	$(LLVM_LD) $(ASM_FLAGS) $(CF) $(TARGET_INC)$(TARGET_ARCH_CC) -c $< -o $@

# Link objects into final executable
$(BIN): $(OBJS)
	@echo "Linking objects into executable $@"
	$(LLVM_CC) $(OBJS) -o $@ $(LD_FLAGS) $(LD_LIB)
	@echo "Built executable: $@"

# Generate the documentation
documentation:
	@echo "Generating documentation..."
	@cp docs/Doxyfile docs/Doxyfile.tmp
	@sed -i 's|^PROJECT_NAME.*|PROJECT_NAME = $(PROJECT_NAME)|' docs/Doxyfile.tmp
	@sed -i 's|^PROJECT_NAME_BRIEF.*|PROJECT_NAME_BRIEF = $(PROJECT_NAME_BRIEF)|' docs/Doxyfile.tmp
	@sed -i 's|^PROJECT_BRIEF.*|PROJECT_BRIEF = $(PROJECT_BRIEF)|' docs/Doxyfile.tmp
	@sed -i 's|^PROJECT_NUMBER.*|PROJECT_NUMBER = $(MAJOR).$(MINOR).$(RELEASE)|' docs/Doxyfile.tmp
	@doxygen docs/Doxyfile.tmp
	@echo "Documentation generated in $(DOC_DIR)"

clean:
	@echo "Cleaning build and documentation directories..."
	@rm -rf $(DOCS_DIR)/html
	@rm -rf $(DOCS_DIR)/man
	@rm -rf $(DOCS_DIR)/Doxyfile.tmp
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

cleanrelease:
	@echo "Cleaning the release directory..."
	@rm -rf release
	
.PHONY: clean