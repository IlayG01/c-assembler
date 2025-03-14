#include <stdio.h>

#include "consts.h"

#define MINIMUM_ARGS 2


int main(int argc, char* argv[]) {
    if (argc < MINIMUM_ARGS) {
        printf("Usage: %s <filename1>.as <filename2>.as ...\n", argv[0]);
        return NO_INPUT_FILES;
    }

    return SUCCESS;
}