#include <stdio.h>

#include "utils.h"
#include "consts.h"
#include "assembler.h"
#include "macro_processor.h"

#define MINIMUM_ARGS 2

/* TODOs */
/* strip whitespaces - between opcodes operands*/
/* walkthrough the error handling in the assebler flow - only fatal errors should stop the program(files access), other should be printed and continue with a flag to not write output files */
/* move out utils functions into utils module */
/* order the code - consts, efficiency, general order*/
/* docs at assembler.c */
/* Fix Warnings */
/* Add examples for input and output files of the program (images), working example and failures with appropriate errors prints */
/* main.c */

int main(int argc, char* argv[]) {
    int i, result;
    char as_file[FILENAME_MAX];
    char am_file[FILENAME_MAX];
    
    if (argc < NO_INPUT_FILES) {
        printf("Usage: %s <file1> [file2] [file3] ...\n", argv[0]);
        return NO_INPUT_FILES;
    }
    
    for (i = 1; i < argc; i++) {
        /* macro process files*/
        copy_filename_with_different_extension(argv[i], as_file, ".as");
        result = macro_process_file(as_file);
        /* assemble files */
        if (result) {
            continue;
        }
        copy_filename_with_different_extension(argv[i], am_file, ".am");
        assemble(am_file);
    }

    return SUCCESS;
}