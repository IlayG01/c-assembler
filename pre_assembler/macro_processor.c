/*
 * Macro Processor - C89 compliant
 * Processes files with macro definitions and usage
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 81        /* 80 chars + null terminator */
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

/* Function prototypes */
int process_file(const char* input_file);
int is_valid_macro_name(const char* name);
int is_reserved_word(const char* name);
int find_macro(const char* name);
void trim_whitespace(char* str);
void strip_newline(char* str);
void add_macro(const char* name);
void add_line_to_macro(int macro_index, const char* line);

int main(int argc, char* argv[]) {
    int i;
    
    if (argc < 2) {
        printf("Usage: %s <file1> [file2] [file3] ...\n", argv[0]);
        return 1;
    }
    
    /* Process each file from command line arguments */
    for (i = 1; i < argc; i++) {
        if (process_file(argv[i]) != 0) {
            printf("Error processing file: %s\n", argv[i]);
        }
    }
    
    return 0;
}

/*
 * Process a single file
 * Returns 0 on success, non-zero on error
 */
int process_file(const char* input_file) {
    FILE* in_file;
    FILE* out_file;
    char line[MAX_LINE_LENGTH];
    char output_file[FILENAME_MAX];
    char macro_name[MAX_MACRO_NAME_LENGTH];
    int in_macro_def = 0;
    int current_macro_index = -1;
    char* token;
    
    /* Check if the file exists */
    in_file = fopen(input_file, "r");
    if (in_file == NULL) {
        printf("File not found: %s\n", input_file);
        return 1;
    }
    
    /* Create output file name with .am extension */
    strcpy(output_file, input_file);
    strcat(output_file, ".am");
    
    out_file = fopen(output_file, "w");
    if (out_file == NULL) {
        printf("Could not create output file: %s\n", output_file);
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
        
        /* Remove leading/trailing whitespace */
        trim_whitespace(line);
        
        /* Check if this is the start of a macro definition */
        if (strncmp(line, "mcro ", 5) == 0) {
            if (in_macro_def) {
                printf("Error: Nested macro definitions not allowed\n");
                continue;
            }
            
            in_macro_def = 1;
            
            /* Extract macro name */
            token = strtok(line + 5, " \t");
            if (token == NULL) {
                printf("Error: Invalid macro definition (no name)\n");
                continue;
            }
            
            strcpy(macro_name, token);
            
            /* Check if there are additional parameters */
            token = strtok(NULL, " \t");
            if (token != NULL) {
                printf("Error: Additional parameters in macro definition line\n");
                continue;
            }
            
            /* Check if macro name is valid */
            if (!is_valid_macro_name(macro_name)) {
                printf("Error: Invalid macro name: %s\n", macro_name);
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
                fprintf(out_file, "%s\n", line);
                continue;
            }
            
            /* Check if there are additional parameters */
            token = strtok(line + 7, " \t");
            if (token != NULL) {
                printf("Error: Additional parameters in macro end line\n");
            }
            
            in_macro_def = 0;
            current_macro_index = -1;
            
            /* Do not write macro end to output file */
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
    }
    
    fclose(in_file);
    fclose(out_file);
    
    return 0;
}

/**
 * Check if a macro name is valid
 * Returns 1 if valid, 0 if invalid
 */
int is_valid_macro_name(const char* name) {
    /* Check if name is empty */
    if (name == NULL || strlen(name) == 0) {
        return 0;
    }
    
    /* Check if name is a reserved word */
    if (is_reserved_word(name)) {
        return 0;
    }
    
    /* Check if name already exists in the macro table */
    if (find_macro(name) >= 0) {
        return 0;
    }
    
    return 1;
}

/**
 * Check if a name is a reserved word
 * Returns 1 if reserved, 0 if not
 */
int is_reserved_word(const char* name) {
    char* reserved_words[] = {
        "stop", "rts", "prn", "red", "jsr", "bne", "jmp", 
        "dec", "inc", "not", "clr", "lea", "sub", "add", "cmp", "mov"
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

/**
 * Find a macro by name
 * Returns the index of the macro in the table, or -1 if not found
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

/*
 * Trim leading and trailing whitespace from a string
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

/*
 * Strip newline characters from the end of a string
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

/*
 * Add a new macro to the table
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

/*
 * Add a line to a macro's definition
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