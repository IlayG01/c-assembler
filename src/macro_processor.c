/*
 * Macro Processor
 * Processes files with macro definitions (as) into target files (am).
 * This program reads an input file, identifies macro definitions, and expands macro invocations in the output file.
 * Non-fatal errors (e.g., file operation failures) are gracefully handled, which might cause additional errors to be encountered.
 *
 */

#include "macro_processor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"

/* Assumptions regarding the length of line and amount of macros in file, their length and their name length */
#define MAX_LINE_LENGTH 81
#define MAX_MACROS 1000
#define MAX_MACRO_LINES 1000
#define MAX_MACRO_NAME_LENGTH 50

/* Macro table structure */
typedef struct {
    char name[MAX_MACRO_NAME_LENGTH];
    char lines[MAX_MACRO_LINES][MAX_LINE_LENGTH];
    int line_count;
} Macro;

/* Global variables */
Macro macro_table[MAX_MACROS];
int macro_count = 0;

/**
 * Initializes the macro table by clearing all entries.
 */
void initialize_macro_table() {
    memset(macro_table, 0, sizeof(macro_table));
    macro_count = 0;
}

/**
 * Finds a macro by its name.
 * 
 * @param name The name of the macro to find.
 * @return The index of the macro in the table, or -1 if not found.
 */
int find_macro(const char* name) {
    int i;
    
    for (i = 0; i < macro_count; i++) {
        if (strcmp(macro_table[i].name, name) == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Strips newline characters ('\n' or '\r') from the end of a string.
 * 
 * @param str The string to process. The operation is performed in-place.
 */
void strip_newline(char* str) {
    char* p = strchr(str, '\n');
    if (p != NULL) {
        *p = '\0';
    }
    
    p = strchr(str, '\r');
    if (p != NULL) {
        *p = '\0';
    }
}

/**
 * Checks if a macro name is valid.
 * 
 * @param name The macro name to validate.
 * @return 1 if the name is valid, 0 otherwise.
 */
int is_valid_macro_name(const char* name) {
    if (name == NULL || strlen(name) == 0) {
        return 0;
    }
    
    if (is_reserved_word(name)) {
        return 0;
    }
    
    if (find_macro(name) >= 0) {
        return 0;
    }
    
    return 1;
}

/**
 * Trims leading and trailing whitespace from a string.
 * 
 * @param str The string to trim. The operation is performed in-place.
 */
void trim_whitespace(char* str) {
    char* start = str;
    char* end;
    
    /* Find the first non-whitespace character */
    while (isspace((unsigned char)*start)) {
        start++;
    }
    
    /* If the string is all whitespace, return an empty string */
    if (*start == '\0') {
        *str = '\0';
        return;
    }
    
    /* Find the last non-whitespace character */
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }
    
    /* Null-terminate the string after the last non-whitespace character */
    *(end + 1) = '\0';
    
    /* If the string didn't start at the beginning, move it there */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

/**
 * Adds a new macro to the macro table.
 * 
 * @param name The name of the macro to add.
 */
void add_macro(const char* name) {
    if (macro_count >= MAX_MACROS) {
        printf("Error: Maximum number of macros reached\n");
        return;
    }
    
    strcpy(macro_table[macro_count].name, name);
    macro_table[macro_count].line_count = 0;
    macro_count++;
}

/**
 * Adds a line to a macro's definition.
 * 
 * @param macro_index The index of the macro in the table.
 * @param line The line to add to the macro.
 */
void add_line_to_macro(int macro_index, const char* line) {
    if (macro_index < 0 || macro_index >= macro_count) {
        return;
    }
    
    if (macro_table[macro_index].line_count >= MAX_MACRO_LINES) {
        printf("Error: Maximum number of lines in macro reached\n");
        return;
    }
    
    strcpy(macro_table[macro_index].lines[macro_table[macro_index].line_count], line);
    macro_table[macro_index].line_count++;
}

int macro_process_file(const char* input_as_file) {
    FILE* in_file;
    FILE* out_file;
    char line[MAX_LINE_LENGTH];
    char output_am_file[FILENAME_MAX];
    char macro_name[MAX_MACRO_NAME_LENGTH];
    int in_macro_def = 0;
    int current_macro_index = -1;
    char* token;
    int is_error_encountered = 0;

    initialize_macro_table();
    
    /* Check if the file exists */
    in_file = fopen(input_as_file, "r");
    if (in_file == NULL) {
        printf("File not found: %s\n", input_as_file);
        return 1;
    }
    
    /* Create output file name with .am extension */
    copy_filename_with_different_extension(input_as_file, output_am_file, ".am");
    
    out_file = fopen(output_am_file, "w");
    if (out_file == NULL) {
        printf("Could not create output file: %s\n", output_am_file);
        fclose(in_file);
        return 1;
    }
    
    /* Process the file line by line */
    while (fgets(line, MAX_LINE_LENGTH, in_file) != NULL) {
        strip_newline(line);
        trim_whitespace(line);
        
        /* Skip empty lines and keep them in output if not in macro definition */
        if (strlen(line) == 0 || (line[0] == ' ' && strlen(line) == 0)) {
            if (!in_macro_def) {
                fprintf(out_file, "%s\n", line);
            }
            continue;
        }
        
        /* Skip comment lines but keep them in output if not in macro definition */
        if (line[0] == ';') {
            if (!in_macro_def) {
                fprintf(out_file, "%s\n", line);
            }
            continue;
        }
        
        /* Check if this is the start of a macro definition */
        if (strncmp(line, "mcro ", 5) == 0) {
            if (in_macro_def) {
                printf("Error: Nested macro definitions not allowed\n");
                is_error_encountered = 1;
                continue;
            }
            
            in_macro_def = 1;
            
            /* Extract macro name */
            token = strtok(line + 5, " \t");
            if (token == NULL) {
                printf("Error: Invalid macro definition (no name)\n");
                is_error_encountered = 1;
                continue;
            }
            
            strcpy(macro_name, token);
            
            /* Check if there are additional parameters */
            token = strtok(NULL, " \t");
            if (token != NULL) {
                printf("Error: Additional parameters in macro definition line\n");
                is_error_encountered = 1;
                continue;
            }
            
            /* Check if macro name is valid */
            if (!is_valid_macro_name(macro_name)) {
                printf("Error: Invalid macro name: %s\n", macro_name);
                is_error_encountered = 1;
                in_macro_def = 0;
                continue;
            }
            
            /* Add macro to the table */
            add_macro(macro_name);
            current_macro_index = macro_count - 1;
            
            /* Do not write macro definition to output file */
            continue;
        }
        
        /* Check if this is the end of a macro definition */
        if (strcmp(line, "mcroend") == 0) {
            if (!in_macro_def) {
                printf("Error: 'mcroend' without matching 'mcro'\n");
                is_error_encountered = 1;
                fprintf(out_file, "%s\n", line);
                continue;
            }
            
            /* Check if there are additional parameters */
            token = strtok(line + 7, " \t");
            if (token != NULL) {
                printf("Error: Additional parameters in macro end line\n");
                is_error_encountered = 1;
            }
            
            in_macro_def = 0;
            current_macro_index = -1;
            
            /* Do not write macroend to output file */
            continue;
        }
        
        if (in_macro_def) {
            /* Add line to the current macro */
            add_line_to_macro(current_macro_index, line);
        } else {
            /* Check if this line is a macro invocation */
            int macro_index = find_macro(line);
            if (macro_index >= 0) {
                /* Replace macro invocation with its content */
                int i;
                for (i = 0; i < macro_table[macro_index].line_count; i++) {
                    fprintf(out_file, "%s\n", macro_table[macro_index].lines[i]);
                }
            } else {
                /* Write the line to the output file as is */
                fprintf(out_file, "%s\n", line);
            }
        }
    }
    
    /* Check if we ended in a macro definition */
    if (in_macro_def) {
        printf("Warning: File ended in macro definition\n");
        is_error_encountered = 1;
    }
    
    fclose(in_file);
    fclose(out_file);

    if (is_error_encountered) {
        if (remove(output_am_file) != 0) {
            printf("Warning: Could not remove output file after error\n");
        }
    }
    
    return is_error_encountered;
}
