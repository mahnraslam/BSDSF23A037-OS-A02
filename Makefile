# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
DEBUG_FLAGS = -g

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Target executable name
TARGET = lsv1.0.0

# Source files
SRC = $(wildcard $(SRC_DIR)/*.c)

# Object files (replace .c with .o and change directory)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Final executable path
EXECUTABLE = $(BIN_DIR)/$(TARGET)

# Default target
all: $(EXECUTABLE)

# Create directories if they don't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link object files to create executable
$(EXECUTABLE): $(OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) -o $@
	@echo "Build successful! Executable: $(EXECUTABLE)"

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build with debug symbols
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(EXECUTABLE)
	@echo "Debug build successful! Use: gdb $(EXECUTABLE)"

# Run the program (for testing)
run: $(EXECUTABLE)
	./$(EXECUTABLE)

# Clean up build artifacts (object files and executable)
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Install to /usr/local/bin (requires sudo)
install: $(EXECUTABLE)
	sudo cp $(EXECUTABLE) /usr/local/bin/
	@echo "Installed to /usr/local/bin/$(TARGET)"

# Uninstall from /usr/local/bin
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled from /usr/local/bin/$(TARGET)"

# Show project structure
tree:
	@echo "Project structure:"
	@tree -I '.git|*.swp|*.o' || find . -type d | sed -e "s/[^-][^\/]*\// |/g" -e "s/|\([^ ]\)/|-\1/"

# Phony targets (not actual files)
.PHONY: all clean run install uninstall debug tree
