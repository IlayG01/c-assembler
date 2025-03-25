# Compiler and flags
CC = gcc
CFLAGS = -Wall -ansi -pedantic
# Source file and target binary
SRC = macro_processor.c
TARGET = macro_processor
# Default makefile target to link and build the executable
all: $(TARGET)
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
# Tests targets for running a test with input file
test:
	chmod +x $(TARGET)
	./$(TARGET) tests/input_files/repetitive_macro.as tests/input_files/empty.as tests/input_files/maman_example.as tests/input_files/multiple_macros.as
# Clean target for cleaning the generated files
clean:
	rm -f $(TARGET)