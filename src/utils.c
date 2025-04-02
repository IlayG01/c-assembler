#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

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

int is_consecutive(char* str, char search_str) {
    int flag = 0;
    while (*str) {
        if (*str == search_str) {
            if (flag) {
                return 1;
            }
            flag = 1;
        } else if (!isspace(*str)) {
            flag = 0;
        }
        str++;
    }
    return 0;
}

int is_reserved_word(const char* name) {
    char* reserved_words[] = {
        "stop", "rts", "prn", "red", "jsr", "bne", "jmp", 
        "dec", "inc", "not", "clr", "lea", "sub", "add", "cmp", "mov", "mcro", "mcroend"
    };
    int i;
    int num_reserved = sizeof(reserved_words) / sizeof(reserved_words[0]);
    
    for (i = 0; i < num_reserved; i++) {
        if (strcmp(name, reserved_words[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

int extend_array(void** array, size_t* current_size, size_t new_size, size_t element_size) {
    void* new_array = realloc(*array, new_size * element_size);
    if (!new_array) {
        return 1; /* Memory allocation failed */
    }
    *array = new_array;
    *current_size = new_size;
    return 0; /* Success */
}