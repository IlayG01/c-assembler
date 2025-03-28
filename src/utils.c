#include "utils.h"

#include <stdio.h>
#include <string.h>

void replace_extension(const char* source_filename, char* target_filename, const char* extension) {
    int len = strlen(source_filename);
    int dot_pos = -1;
    int i;

    /* Find the position of the last dot (.) */
    for (i = len - 1; i >= 0; i--) {
        if (source_filename[i] == '.') {
            dot_pos = i;
            break;
        }
    }

    /* Copy the source filename into the target filename (up to the dot or full filename if no dot) */
    if (dot_pos > 0) {
        /* Copy the part before the dot */
        strncpy(target_filename, source_filename, dot_pos);
        target_filename[dot_pos] = '\0';  /* Null-terminate the string at the dot position */
    } else {
        /* If no dot, copy the entire filename */
        strcpy(target_filename, source_filename);
    }

    /* Append the new extension to the target filename */
    strcat(target_filename, extension);

    return;
}