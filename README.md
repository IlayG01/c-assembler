
# Open University C-Assembler Project
This project implements a two-pass assembler for a custom assembly language. It processes assembly files (`.as`) and generates machine code, along with additional files for entries and externals. The assembler also includes a macro processor to handle macro definitions and expansions.

## Usage

1. **Build the Assembler**  
   Use the provided `Makefile` to compile the project:
   ```sh
   make
   ```

2. **Run the Assembler**  
   To process one or more assembly files, use the following command:
   ```sh
   ./assembler <file1> [file2] [file3] ...
   ```
   Replace `<file1>`, `<file2>`, etc., with the base names of your `.as` files (without the extension).

3. **Test the Assembler**  
   Run the provided test cases:
   ```sh
   make test
   ```

4. **Clean Up**  
   To remove generated files and binaries:
   ```sh
   make clean
   ```

   To clean only the test-generated files:
   ```sh
   make clean_test
   ```

## Notes

- **Input Files**:  
  The assembler expects input files with the `.as` extension. These files can include macro definitions, assembly instructions, and directives such as `.data`, `.string`, `.entry`, and `.extern`.

- **Output Files**:  
  For each input file, the assembler generates the following:
  - `.am`: Preprocessed file with expanded macros.
  - `.obj`: Machine code file.
  - `.ent`: File listing entry labels and their addresses.
  - `.ext`: File listing external labels and their usage addresses.

- **Macro Processor**:  
  The macro processor expands macros defined using `mcro` and `mcroend`. Nested macros and invalid macro names are not allowed.

- **Assembly Process**:  
  The assembler operates in two cycles:
  1. **First Cycle**: Parses the input file, builds the symbol table, and translates data and code sections.
  2. **Second Cycle**: Resolves symbols and generates the final machine code.

- **Testing**:  
  The `tests/input_files` directory contains various test cases to validate the assembler's functionality. These include valid assembly files, files with errors, and edge cases. The `images` directory includes visual example from tests.

- **Dependencies**:  
  The project uses standard C libraries and does not require any external dependencies.

- **Compiler Flags**:  
  The project is compiled with the following flags for strict **ANSI C** compliance:
  - `-Wall`: Enable all warnings.
  - `-pedantic`: Enforce strict ANSI C compliance.
  - `-ansi`: Use the ANSI C standard.

- **Authors**
- Ido Ben Hamo
- Ilay Gilman