# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -pedantic -ansi -g

# Source files for both projects
SRC = src/main.c src/assembler.c src/macro_processor.c src/utils.c

# Object files
OBJ = $(SRC:.c=.o)

# Executable name
TARGET_ASSEMBLER = assembler
TARGET_MACRO_PROCESSOR = macro_processor

# Default target to build the executable
all: $(TARGET_ASSEMBLER)

$(TARGET_ASSEMBLER): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET_ASSEMBLER)
	rm $(OBJ)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Macro Processor (Standalone for testability)
$(TARGET_MACRO_PROCESSOR): src/macro_processor.c
	$(CC) $(CFLAGS) -o $(TARGET_MACRO_PROCESSOR) src/macro_processor.c

# Clean target to clean the generated files
clean:
	rm -f $(OBJ) $(TARGET_ASSEMBLER) $(TARGET_MACRO_PROCESSOR)

# Run the assembler
run: all
	./$(TARGET_ASSEMBLER) $(ARGS)

# Test the macro processor independently
test_macro_processor: $(TARGET_MACRO_PROCESSOR)
	chmod +x $(TARGET_MACRO_PROCESSOR)
	./$(TARGET_MACRO_PROCESSOR) tests/input_files/repetitive_macro.as tests/input_files/empty.as tests/input_files/maman_example.as tests/input_files/multiple_macros.as

# PHONY targets
.PHONY: all clean run test_macro_processor