# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -pedantic -ansi -g

# Source files
SRC = src/main.c src/assembler.c

# Object files
OBJ = $(SRC:.c=.o)

# Executable name
TARGET = assembler

# Link the executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)
	rm $(OBJ)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean compiled files
clean:
	rm -f $(OBJ) $(TARGET)

# Run the program
run: all
	./$(TARGET)

# PHONY targets
.PHONY: all clean run