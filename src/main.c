#include <stdio.h>

#include "utils.h"
#include "consts.h"
#include "assembler.h"
#include "macro_processor.h"

#define MINIMUM_ARGS 2

/* TODO */
/* strip whitespaces - between opcodes operands*/
/* errors  + error handling - prob linked list */
/* split to files */
/* order the code - consts, efficiency, general order*/
/* DOCS */
/* Fix Warnings */
/* main.c */

int main(int argc, char* argv[]) {
    int i;
    char am_file[FILENAME_MAX];
    
    if (argc < NO_INPUT_FILES) {
        printf("Usage: %s <file1.as> [file2.as] [file3.as] ...\n", argv[0]);
        return NO_INPUT_FILES;
    }
    
    /* macro process files*/
    for (i = 1; i < argc; i++) {
        macro_process_file(argv[i]);
    }
    /* assemble files */
    for (i = 1; i < argc; i++) {
        replace_extension(argv[i], am_file, ".am");
        assemble(am_file);
    }

    return SUCCESS;
}