#include <stdio.h>

#include "utils.h"
#include "consts.h"
#include "assembler.h"
#include "macro_processor.h"

#define MINIMUM_ARGS 2


int main(int argc, char* argv[]) {
    int i, result;
    char as_file[FILENAME_MAX];
    char am_file[FILENAME_MAX];
    
    if (argc < MINIMUM_ARGS) {
        printf("Usage: %s <file1> [file2] [file3] ...\n", argv[0]);
        return NO_INPUT_FILES;
    }
    
    for (i = 1; i < argc; i++) {
        /* macro process files*/
        copy_filename_with_different_extension(argv[i], as_file, ".as");
        printf("### Starting processing on file %s ###\n", as_file);
        result = macro_process_file(as_file);
        /* assemble files */
        if (result) {
            continue;
        }
        copy_filename_with_different_extension(argv[i], am_file, ".am");
        assemble(am_file);
        printf("### Finished processing on file %s ###\n", as_file);
    }

    return SUCCESS;
}