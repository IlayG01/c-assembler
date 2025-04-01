# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -pedantic -ansi

# Source files
ASSEMBLER_SRC = src/main.c src/assembler.c src/macro_processor.c src/utils.c

# Object files
ASSEMBLER_OBJ = $(ASSEMBLER_SRC:.c=.o)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Executable name
TARGET_ASSEMBLER = assembler

# Default target to build the executables
all: $(TARGET_ASSEMBLER)

$(TARGET_ASSEMBLER): $(ASSEMBLER_OBJ)
	$(CC) $(ASSEMBLER_OBJ) -o $(TARGET_ASSEMBLER)
	rm $(ASSEMBLER_OBJ)


# Clean target to clean the generated files
clean: clean_test
	rm -f $(ASSEMBLER_OBJ) $(TARGET_ASSEMBLER)

# Run the assembler
run: all
	./$(TARGET_ASSEMBLER) $(ARGS)

# Test related files
BASE_FILES = tests/input_files/repetitive_macro tests/input_files/empty tests/input_files/maman_macro_example tests/input_files/maman_cycle_example tests/input_files/multiple_macros
CREATED_EXTENSIONS = .am .ent .obj .ext

# Test the assembler
test: $(TARGET_ASSEMBLER)
	chmod +x $(TARGET_ASSEMBLER)
	./$(TARGET_ASSEMBLER) $(BASE_FILES)

clean_test:
	@echo "Cleaning up generated test files..."
	# Iterate over each input base and remove the files with the relevant extensions
	@for base in $(BASE_FILES); do \
		for ext in $(CREATED_EXTENSIONS); do \
			if [ -f $$base$$ext ]; then \
				rm -f $$base$$ext; \
			fi \
		done; \
	done


# PHONY targets
.PHONY: all clean run test clean_test