#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* copies the filename from the source filename into the target filename with a different extension */
void copy_filename_with_different_extension(const char* source_filename, char* target_filename, const char* extension) {
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
        strncpy(target_filename, source_filename, dot_pos);
        target_filename[dot_pos] = '\0';  /* Null-terminate the string at the dot position */
    } else {
        strcpy(target_filename, source_filename);
    }

    /* Append the new extension to the target filename */
    strcat(target_filename, extension);

    return;
}

void strip_whitespace(char *str) {
    int i;
    int start = 0;
    int end = strlen(str) - 1;

    /* Trim leading whitespace */
    while (isspace((unsigned char)str[start])) {
        start++;
    }

    /* Trim trailing whitespace */
    while (end > start && isspace((unsigned char)str[end])) {
        end--;
    }

    /* Shift characters forward */
    for (i = 0; i <= end - start; i++) {
        str[i] = str[start + i];
    }
    str[i] = '\0'; /* Null-terminate the string */
}
