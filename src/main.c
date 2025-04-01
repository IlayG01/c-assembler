#include <stdio.h>

#include "utils.h"
#include "consts.h"
#include "assembler.h"
#include "macro_processor.h"

#define MINIMUM_ARGS 2

/* TODOs */
/* data and externals should be aloocated dynamically */
/* docs */
/* Fix Warnings in pre processor */
/* Add examples for input and output files of the program (images), working example and failures with appropriate errors prints */
/* update main.c */
/* Solve TODO comments in code */

int main(int argc, char* argv[]) {
    int i;
    char am_file[FILENAME_MAX];
    
    if (argc < MINIMUM_ARGS) {
        printf("Usage: %s <file1.as> [file2.as] [file3.as] ...\n", argv[0]);
        return NO_INPUT_FILES;
    }
    
    /* macro process files*/
    for (i = 1; i < argc; i++) {
        macro_process_file(argv[i]);
    }
    /* assemble files */
    for (i = 1; i < argc; i++) {
        copy_filename_with_different_extension(argv[i], am_file, ".am");
        assemble(am_file);
    }

    return SUCCESS;
}