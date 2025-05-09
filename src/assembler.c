/**
 * This file implements the main functionality of a two-pass assembler for a custom assembly language.
 * It includes functions for parsing assembly code, validating instructions, managing symbol tables,
 * and generating machine code. The assembler supports directives such as `.data`, `.string`, `.entry`, 
 * and `.extern`, as well as various addressing modes for instructions.
 * 
 * The assembler operates in two cycles:
 * 1. First Cycle: Parses the input file, builds the symbol table, and translates data and code sections.
 * 2. Second Cycle: Resolves symbols and generates the final machine code.
 * 
 * Outputs:
 * - Object file (.obj): Contains the machine code.
 * - Entries file (.ent): Lists entry labels and their addresses.
 * - Externals file (.ext): Lists external labels and their usage addresses.
 */

#include "assembler.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "utils.h"
#include "consts.h"

#define MAX_BUF_SIZE 100
#define MAX_INSTRUCTIONS 1000
#define LINE_MAX_SIZE 80
#define CODE_BASE_ADDRESS 100
#define IMMEDIATE_ADDRESS_MODE 0
#define DIRECT_ADDRESS_MODE 1
#define REALTIVE_ADDRESS_MODE 2
#define REGISTER_ADDRESS_MODE 3


/**
 * Checks if a label is valid according to the assembler's rules.
 * 
 * @param label The label to validate.
 * @return 1 if the label is valid, 0 otherwise.
 */
int is_valid_label(const char *label) {
    int len = strlen(label);
    int i;

    if (is_reserved_word(label)) {
        return 1;
    }

    /* Check length constraint */
    if (len == 0 || len > MAX_LABEL_LENGTH) {
        return 0;
    }

    /* Check first character (must be a letter) */
    if (!isalpha(label[0])) {
        return 0;
    }

    /* Check remaining characters (must be alphanumeric) */
    for (i = 1; i < len; i++) {
        if (!isalnum(label[i])) {
            return 0;
        }
    }

    return 1;
}

/**
 * Extracts a label from a line of assembly code. Assumes the label ends with ':'.
 * 
 * @param line The line containing the label.
 * @param label The buffer to store the extracted label.
 */
void get_label(char* line, char* label) {
    /* Find the position of ':' */
    char *colon_pos = strchr(line, ':');

    /* Extract the label */
    int label_length = colon_pos - line;
    
    /* Copy the label to a separate string and remove trailing spaces */
    strncpy(label, line, label_length);
    label[label_length] = '\0';
}

/**
 * Checks if a label exists in the symbol table.
 * 
 * @param label The label to check.
 * @param label_table The symbol table.
 * @param label_count The number of labels in the table.
 * @return 1 if the label exists, 0 otherwise.
 */
int is_label_exist(char* label, label_element* label_table, size_t label_count) {
    int i;
    for (i = 0; i < label_count; i++)
    {
        if (!strcmp(label_table[i].label_name, label)) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * @param ins The instruction string.
 * @return 1 if the instruction is a `.data` directive, 0 otherwise.
 */
int is_data_instruction(char* ins) {
    return NULL != strstr(ins, ".data");
}

/**
 * @param ins The instruction string.
 * @return 1 if the instruction is a `.entry` directive, 0 otherwise.
 */
int is_entry_instruction(char* ins) {
    return NULL != strstr(ins, ".entry");
}

/**
 * @param ins The instruction string.
 * @return 1 if the instruction is a `.extern` directive, 0 otherwise.
 */
int is_extern_instruction(char* ins) {
    return NULL != strstr(ins, ".extern");
}

/**
 * @param ins The instruction string.
 * @return 1 if the instruction is a `.string` directive, 0 otherwise.
 */
int is_string_instruction(char* ins) {
    return NULL != strstr(ins, ".string");
}

/**
 * Adds a label to the symbol table, reallocating memory as needed.
 * 
 * @param label_table Pointer to the symbol table.
 * @param label_count Pointer to the number of labels in the table.
 * @param label The label to add.
 * @param address The address associated with the label.
 * @param label_type The type of the label (e.g., data, code, extern).
 * @return SUCCESS on success, MEMORY_ALLOCATION_FAILED on failure.
 */
int add_label_to_symbol_table(label_element** label_table, size_t* label_count, char* label, size_t address, label_options label_type) {
    char* label_copy;
    size_t current_label_count = *label_count;
    /* Allocate memory for the new label table (increase the size) */
    if (extend_array((void**)label_table, label_count, current_label_count + 1, sizeof(label_element))) {
        return MEMORY_ALLOCATION_FAILED;
    }

    /* Assign values to the new label entry */
    (*label_table)[current_label_count].address = address;
    (*label_table)[current_label_count].label_type = label_type;

    /* Allocate memory for the label name and copy it */
    label_copy = (char*)malloc(strlen((label)) + 1);
    if (!label_copy) {
        return MEMORY_ALLOCATION_FAILED;
    }
    strcpy(label_copy, label);

    (*label_table)[current_label_count].label_name = label_copy;

    return SUCCESS; /* Success */
}

/**
 * Parses a `.data` directive and populates the data array with integer values.
 * 
 * @param data The data array to populate.
 * @param count Pointer to the current count of data entries.
 * @param line The line containing the `.data` directive.
 * @return SUCCESS on success, 1 on failure.
 * 
 */
int translate_data(data** data_table, size_t* count, char* line) {
    char *token = strtok(line, " \t");
    int value;
    size_t temp_count;

    if (!token || strcmp(token, ".data")) return 1; /* Ensure it's a `.data` directive */

    while ((token = strtok(NULL, ",")) != NULL) {
        strip_whitespace(token);
        if (strlen(token) == 0) {
            return 1;
        }
        value = atoi(token); /* Convert the token into an integer */

        temp_count = *count;
        if (extend_array((void**)data_table, count, *count + 1, sizeof(data))) {
            return 1;
        }

        (*data_table)[temp_count].value.integer = value; /* Use the `data` struct's integer field */
    }
    return SUCCESS; /* Success */
}

/**
 * Parses a `.string` directive and populates the data array with ASCII values.
 * 
 * @param data The data array to populate.
 * @param count Pointer to the current count of data entries.
 * @param line The line containing the `.string` directive.
 * @return SUCCESS on success, 1 on failure.
 */
int translate_string(data** data_table, size_t* count, char* line) {
    size_t str_len;
    int i;
    size_t temp_count; 
    char *token = strtok(line, "\""); /* Tokenize by " */
    strip_whitespace(token);
    if (!token || strcmp(token, ".string")) return 1; /* Ensure it's a `.string` directive */

    token = strtok(NULL, "\""); /* Get the string inside quotes */
    if (!token) {
        return 1;  /* if "" not provided */
    }

    str_len = strlen(token);
    temp_count = *count;
    if (extend_array((void**)data_table, count, *count + str_len + 1, sizeof(data))) {
        return 1;
    }

    for (i = 0; i < str_len; i++) {
        (*data_table)[temp_count].value.ascii = (int)token[i]; /* Use the `data` struct's integer field */
        temp_count++;
    }
    (*data_table)[temp_count].value.ascii = 0; /* null */

    return SUCCESS; /* Success */
}

/**
 * @param str The string representation of the opcode.
 * @return The corresponding opcode enum value, or INVALID if not found.
 */
opcode get_opcode(const char* str) {
    int i;
    for (i = 0; i < INVALID; i++) {
        if (!strcmp(str, OPCODE_STRINGS[i])) {
            return (opcode)i;
        }
    }
    return INVALID;  /* Return INVALID if not found */
}

/**
 * Parses an instruction line, extracting the opcode and operands.
 * 
 * @param instr The instruction structure to populate.
 * @param line The line containing the instruction.
 */
void parse_instruction(instruction* instr, const char* line) {
    char buffer[MAX_BUF_SIZE];
    char* token;
    int i = 0;

    instr->num_of_operands = 0;
    
    strcpy(buffer, line);  /* Copy to modify safely */
    token = strtok(buffer, " \t");  /* First token (opcode) */
    if (!token) {
        instr->opcode = INVALID;
        return;
    }

    instr->opcode = get_opcode(token);
    if (instr->opcode == INVALID) {
        return;
    }
    
    /* Extract operands */
    while (i < MAX_OPERANDS && (token = strtok(NULL, ","))) {
        strip_whitespace(token);
        strcpy(instr->operands[i], token);
        instr->num_of_operands++;
        i++;
    }

    if (strtok(NULL, ",") != NULL) {
        instr->opcode = INVALID;  /* Mark as invalid */
    }
}

/**
 * Identifies the addressing mode of an operand.
 * 
 * @param operand The operand string.
 * @return The addressing mode of the operand.
 * 
 */
int get_addressing_mode(const char* operand) {
    if (!operand) return -1;
    if (operand[0] == '#') return IMMEDIATE_ADDRESS_MODE;
    if (operand[0] == '&') return REALTIVE_ADDRESS_MODE;
    if (operand[0] == 'r' && strlen(operand) == 2 && operand[1] >= '0' && operand[1] <= '7') return REGISTER_ADDRESS_MODE;
    return DIRECT_ADDRESS_MODE;
}

/**
 * Checks if an addressing mode is allowed for an operand.
 * 
 * @param mode The addressing mode to check.
 * @param allowed_modes Array of allowed addressing modes.
 * @param num_modes The number of allowed modes.
 * @return 1 if the mode is allowed, 0 otherwise.
 */
int is_mode_allowed(int mode, const int allowed_modes[], int num_modes) {
    int i;
    for (i = 0; i < num_modes; i++) {
        if (allowed_modes[i] == mode) return 1;
    }
    return 0;
}

/**
 * Retrieves the rule for a given opcode.
 * 
 * @param opcode The opcode to look up.
 * @return Pointer to the corresponding opcode rule, or NULL if not found.
 */
const OpcodeRule* get_opcode_rule(opcode opcode) {
    int i;
    for (i = 0; i < OPCODE_TABLE_SIZE; i++) {
        if (OPCODE_TABLE[i].opcode == opcode) {
            return &OPCODE_TABLE[i];  /* Return pointer to found rule */
        }
    }
    return NULL;  /* Return NULL if not found */
}

/**
 * Validates an instruction against its opcode rule and addressing modes.
 * 
 * @param instr The instruction to validate.
 * @return SUCCESS if the instruction is valid, or an error code otherwise.
 */
int validate_instruction(const instruction* instr) {
    /* Find the corresponding opcode rule */
    const OpcodeRule* rule = get_opcode_rule(instr->opcode);
    if (!rule) {
        return INVALID_OPCODE;
    }

    /* Check number of operands */
    if (instr->num_of_operands != rule->num_of_operands) {
        return WRONG_AMOUNT_OF_OPERANDS;
    }

    /* Validate addressing modes correctly */
    if (instr->num_of_operands == 2) {
        /* Instruction has both source and destination operands */
        int src_mode = get_addressing_mode(instr->operands[0]);
        int dest_mode = get_addressing_mode(instr->operands[1]);

        if (!is_mode_allowed(src_mode, rule->valid_source_modes, rule->num_source_modes)) {
            return INVALID_SRC_OPERAND_ADDRESSING_MODE;
        }
        if (!is_mode_allowed(dest_mode, rule->valid_dest_modes, rule->num_dest_modes)) {
            return INVALID_DST_OPERAND_ADDRESSING_MODE;
        }
    } else if (instr->num_of_operands == 1) {
        /* Instruction only has a destination operand */
        int dest_mode = get_addressing_mode(instr->operands[0]);

        if (!is_mode_allowed(dest_mode, rule->valid_dest_modes, rule->num_dest_modes)) {
            return INVALID_DST_OPERAND_ADDRESSING_MODE;
        }
    }

    return SUCCESS;  /* Valid instruction */
}

/**
 * Writes the first word of machine code in hexadecimal format to a file.
 * 
 * @param file The file pointer to write to.
 * @param first_word The first word structure to write.
 */
void write_first_word_hex_to_file(FILE* file, first_word* first_word) {
    unsigned int value = 0;

    value |= (first_word->E            & 0x1)      << 0;   
    value |= (first_word->R            & 0x1)      << 1;   
    value |= (first_word->A            & 0x1)      << 2;   
    value |= (first_word->funct        & 0x1F)     << 3;   
    value |= (first_word->dest_reg     & 0x7)      << 8;   
    value |= (first_word->dest_address & 0x3)      << 11;  
    value |= (first_word->src_reg      & 0x7)      << 13;  
    value |= (first_word->src_address  & 0x3)      << 16;  
    value |= (first_word->opcode_value & 0x3F)     << 18;  

    fprintf(file, "%06X\n", value & 0xFFFFFF);
}

/**
 * Writes an operand of machine code in hexadecimal format to a file.
 * 
 * @param file The file pointer to write to.
 * @param operand The operand structure to write.
 */
void write_operand_hex_to_file(FILE* file, operand* operand) {
    unsigned int value = 0;

    value |= (operand->E        & 0x1)       << 0;   
    value |= (operand->R        & 0x1)       << 1;   
    value |= (operand->A        & 0x1)       << 2;   
    value |= (operand->integer  & 0x1FFFFF)  << 3;   

    fprintf(file, "%06X\n", value & 0xFFFFFF);
}

/**
 * Generates the first word of machine code for an instruction.
 * 
 * @param instr The instruction to generate the first word for.
 * @return The generated first word structure.
 */
first_word generate_first_word(const instruction* instr) {
    const OpcodeRule* opcode_rule = get_opcode_rule(instr->opcode);
    first_word first_word_val;

    first_word_val.A = 1;
    first_word_val.R = 0;
    first_word_val.E = 0;
    first_word_val.opcode_value = opcode_rule->opcode_value;
    first_word_val.funct = opcode_rule->funct;
    if (instr->num_of_operands == 2) {
        first_word_val.src_address = get_addressing_mode(instr->operands[0]);
        if (first_word_val.src_address == REGISTER_ADDRESS_MODE) {
            first_word_val.src_reg = instr->operands[0][1] - '0';
        } else {
            first_word_val.src_reg = 0;
        }
        first_word_val.dest_address = get_addressing_mode(instr->operands[1]);
        if (first_word_val.dest_address == REGISTER_ADDRESS_MODE) {
            first_word_val.dest_reg = instr->operands[1][1] - '0';
        } else {
            first_word_val.dest_reg = 0;
        }
    } else if (instr->num_of_operands == 1) {
        first_word_val.src_address = 0;
        first_word_val.src_reg = 0;
        first_word_val.dest_address = get_addressing_mode(instr->operands[0]);
        if (first_word_val.dest_address == REGISTER_ADDRESS_MODE) {
            first_word_val.dest_reg = instr->operands[0][1] - '0';
        } else {
            first_word_val.dest_reg = 0;
        }
    } else {
        first_word_val.src_address = 0;
        first_word_val.src_reg = 0;
        first_word_val.dest_address = 0;
        first_word_val.dest_reg = 0;
    }

    return first_word_val;
}

/**
 * Calculates the number of words required for an instruction, including operands.
 * 
 * @param instr The instruction to calculate the number of words for.
 * @return The number of words required for the instruction.
 */
int calculate_number_of_words(const instruction* instr) {
    if (instr->num_of_operands == 2) {
        /* Instruction has both source and destination operands */
        int src_mode = get_addressing_mode(instr->operands[0]);
        int dest_mode = get_addressing_mode(instr->operands[1]);

        return 1 + (src_mode != REGISTER_ADDRESS_MODE) + (dest_mode != REGISTER_ADDRESS_MODE);
    } else if (instr->num_of_operands == 1) {
        /* Instruction only has a destination operand */
        int dest_mode = get_addressing_mode(instr->operands[0]);
        return 1 + (dest_mode != REGISTER_ADDRESS_MODE);
    }
    return 1;
}

/**
 * Generates the machine code for an operand.
 * 
 * @param operand_val The operand string to generate code for.
 * @return The generated operand structure.
 */
operand generate_operand_code(char* operand_val) {
    operand operand = {0};
    operand.A = 1;
    operand.R = 0;
    operand.E = 0;
    operand.integer = atoi(operand_val+1); /* skip the # */
    return operand;
}

/**
 * Builds the machine code for an instruction, resolving immediate operands.
 * 
 * @param instr The instruction to build.
 * @param machine_code The machine code structure to populate.
 * @return The number of operands resolved.
 */
int build_instruction(instruction* instr, machine_code* machine_code) {
    int i;
    int amount_opernads_resolved = 0;
    int operand_code_index = 0;
    int address_mode;

    machine_code->first_word_val = generate_first_word(instr);
    for (i = 0; i < instr->num_of_operands; i++) {
        address_mode = get_addressing_mode(instr->operands[i]);
        if (address_mode == IMMEDIATE_ADDRESS_MODE) {
            amount_opernads_resolved++;
            machine_code->operand_code[operand_code_index] = generate_operand_code(instr->operands[i]);
        }
        if (address_mode != REGISTER_ADDRESS_MODE) {
            operand_code_index++;  /* no additional word for reg address */
        }
    }
    return amount_opernads_resolved;
}

/**
 * Saves the object file (.obj) containing the machine code.
 * 
 * @param filename The name of the assembly file.
 * @param code The machine code array.
 * @param code_count The number of machine code entries.
 * @param data The data array.
 * @param data_count The number of data entries.
 * @param ICF The final instruction counter value.
 * @param DCF The final data counter value.
 */
void save_obj_file(const char* filename, machine_code* code, size_t code_count, data* data, size_t data_count, size_t ICF, size_t DCF) {
    char obj_filename[FILENAME_MAX];
    FILE* file = NULL;
    int line_number = CODE_BASE_ADDRESS;
    int i, j;

    copy_filename_with_different_extension(filename, obj_filename, ".obj");
    file = fopen(obj_filename, "w");

    fprintf(file, "%7ld %ld\n", ICF-CODE_BASE_ADDRESS, DCF);
    for (i = 0; i < code_count; i++)
    {
        fprintf(file, "%07d ", line_number++);
        write_first_word_hex_to_file(file, &code[i].first_word_val);

        for (j = 0; j < code[i].L-1; j++)
        {
            fprintf(file, "%07d ", line_number++);
            write_operand_hex_to_file(file, &code[i].operand_code[j]);
        }
    }
    for (i = 0; i < data_count; i++)
    {
        fprintf(file, "%07d %06X\n", line_number++, data[i].value.integer);
    }

    fclose(file);
}

/**
 * Saves the entries file (.ent) listing entry labels and their addresses.
 * 
 * @param filename The name of the assembly file.
 * @param label_table The symbol table.
 * @param label_count The number of labels in the table.
 */
void save_entries_file(const char* filename, label_element* label_table, size_t label_count) {
    char ent_filename[FILENAME_MAX];
    FILE* file = NULL;
    int i;

    copy_filename_with_different_extension(filename, ent_filename, ".ent");
    file = fopen(ent_filename, "w");

    for (i = 0; i < label_count; i++) {
        if (label_table[i].label_type & entry_label) {
            fprintf(file, "%s %07d\n", label_table[i].label_name, label_table[i].address);
        }
    }

    fclose(file);
}

/**
 * Saves the externals file (.ext) listing external labels and their usage addresses.
 * 
 * @param filename The name of the assembly file.
 * @param externals The externals array.
 * @param externals_count The number of externals.
 */
void save_externals_file(const char* filename, external_info* externals, size_t externals_count) {
    char ext_filename[FILENAME_MAX];
    FILE* file = NULL;
    int i;

    copy_filename_with_different_extension(filename, ext_filename, ".ext");
    file = fopen(ext_filename, "w");

    for (i = 0; i < externals_count; i++) {
        fprintf(file, "%s %07d\n", externals[i].label_name, externals[i].address);
    }

    fclose(file);
}

/**
 * Performs the second cycle of the assembly process
 * 
 * @param file The file pointer to the assembly file.
 * @param label_table The symbol table.
 * @param label_count The number of labels in the table.
 * @param code The machine code array.
 * @param code_count The number of machine code entries.
 * @param externals The externals array to populate.
 * @param externals_count Pointer to the count of externals.
 * @return 0 on success, 1 if errors were encountered.
 */
int second_cycle(FILE* file, label_element* label_table, size_t label_count, machine_code* code, size_t code_count, external_info** externals, size_t* externals_count) {
    char line[MAX_BUF_SIZE];  /* Line Max Size = 80 */
    char label[MAX_LABEL_LENGTH + 1] = {0};
    int code_line_number = 0;
    int line_number = 0;
    int is_code_with_errors = 0;
    int i, j;
    char* label_copy;
    char* mod_line;

    rewind(file);
    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;
        strip_whitespace(line);

        if (line[0] == ';' || strlen(line) == 0) {
            /* comment - skip */
            continue;
        }

        mod_line = line;
        if (strchr(line, ':')) {
            get_label(line, label);  /* cant be wrong label - because we check in first cycle */
            mod_line += strlen(label) + 1;
        }

        strip_whitespace(mod_line);
        
        if (is_data_instruction(mod_line) || is_string_instruction(mod_line) || is_extern_instruction(mod_line)) {
            continue;
        }
        if (is_entry_instruction(mod_line)) {
            char* token = strtok(mod_line, " \t"); /* Tokenize by space or tab */
            int found = 0;
            if (!token || strcmp(token, ".entry")) {
                printf("Error: Invalid entry line. Line number (%d)\n", line_number);
                is_code_with_errors = 1;
                continue;
            }
            token = strtok(NULL, " \t"); /* Get the next token, which is the name */
            if (is_reserved_word(token)) {
                printf("Error: Invalid entry label (%s) encountered.\n", token);
                is_code_with_errors = 1;
                continue;
            }

            for (i = 0; i < label_count; i++)
            {
                if (!strcmp(token, label_table[i].label_name)) {
                    label_table[i].label_type |= entry_label;
                    found = 1;
                }
            }
            if (found) {
                continue;
            }
            
            printf("Error: Entry Label (%s) doesn't exists.\n", token);
            is_code_with_errors = 1;
            continue;
        }

        if (code[code_line_number].need_to_resolve) {
            instruction instr;
            size_t temp_count;
            int address_mode;
            int operand_code_index = 0;
            int label_type;
            int label_address;
            char* label_name;

            parse_instruction(&instr, mod_line);
            for (i = 0; i < instr.num_of_operands; i++) {
                address_mode = get_addressing_mode(instr.operands[i]);
                if (address_mode == IMMEDIATE_ADDRESS_MODE) {
                    operand_code_index++;  /* already built */
                    continue;
                }
                if (address_mode == REGISTER_ADDRESS_MODE) {
                    continue;  /* no additional word for reg address */
                }

                label_name = instr.operands[i];
                if (address_mode == REALTIVE_ADDRESS_MODE) {
                    /* contain & as prefix */
                    label_name++;
                }
                if (!is_label_exist(label_name, label_table, label_count)) {
                    printf("Error: Label (%s) doesn't exists.\n", label_name);
                    is_code_with_errors = 1;
                    continue;
                }

                for (j = 0; j < label_count; j++)
                {
                    if (!strcmp(label_name, label_table[j].label_name)) {
                        label_address = label_table[j].address;
                        label_type = label_table[j].label_type;
                        break;
                    }
                }

                if (label_type == extern_label) {
                    if (address_mode == REALTIVE_ADDRESS_MODE) {
                        printf("Error: Invalid jump to external address (%s).\n", label_name);
                        is_code_with_errors = 1;
                        continue;
                    }

                    temp_count = *externals_count;
                    if (extend_array((void**)externals, externals_count, *externals_count + 1, sizeof(external_info))) {
                        printf("Error: Memory allocation failed.\n");
                        is_code_with_errors = 1;
                        continue;
                    }
                    (*externals)[temp_count].address = code[code_line_number].IC + 1 + operand_code_index;

                    label_copy = (char*)malloc(strlen((label_name)) + 1);
                    if (!label_copy) {
                        printf("Error: Memory allocation failed.\n");
                        is_code_with_errors = 1;
                        continue;
                    }
                    strcpy(label_copy, label_name);
                    (*externals)[temp_count].label_name = label_copy;

                    code[code_line_number].operand_code[operand_code_index].A = 0;
                    code[code_line_number].operand_code[operand_code_index].R = 0;
                    code[code_line_number].operand_code[operand_code_index].E = 1;
                    code[code_line_number].operand_code[operand_code_index].integer = 0;
                } else {
                    code[code_line_number].operand_code[operand_code_index].E = 0;
                    if (address_mode == REALTIVE_ADDRESS_MODE) {
                        code[code_line_number].operand_code[operand_code_index].A = 1;                        
                        code[code_line_number].operand_code[operand_code_index].R = 0;
                        code[code_line_number].operand_code[operand_code_index].integer = label_address - code[code_line_number].IC;
                    } else {
                        /* address mode == DIRECT_ADDRESS_MODE */
                        code[code_line_number].operand_code[operand_code_index].A = 0;
                        code[code_line_number].operand_code[operand_code_index].R = 1;
                        code[code_line_number].operand_code[operand_code_index].integer = label_address;
                    }
                }
                operand_code_index++;
            }
            
        }
        code_line_number++;
    }

    return is_code_with_errors;
}

/**
 * Performs the first cycle of the assembly process
 * 
 * @param filename The name of the assembly file to process.
 */
void first_cycle(char* filename) {
    char line[MAX_BUF_SIZE];
    char label[MAX_LABEL_LENGTH + 1] = {0};
    int last_error;
    int is_code_with_errors = 0;
    int i;
    int amount_opernads_resolved;
    int L;
    int line_number = 0;
    int is_line_with_label = 0;
    char* mod_line;
    char* token;
    size_t IC = CODE_BASE_ADDRESS, DC = 0, ICF, DCF;
    instruction ins;
    
    machine_code code[MAX_INSTRUCTIONS];
    data* data = NULL;
    external_info* externals = NULL;
    label_element* label_table = NULL;
    size_t label_count = 0, data_count = 0, code_count = 0, externals_count = 0;
    size_t data_count_temp;

    FILE *file;

    file = fopen(filename, "r");
    if (!file) {
        printf("Error: The specified file (%s) does not exist.\n", filename);
        return;
    }
    
    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;
        if (strlen(line) > LINE_MAX_SIZE) {
            printf("Error: Line number: (%d) too long.\n", line_number);
            is_code_with_errors = 1;
            continue;
        }
        strip_whitespace(line);

        if (line[0] == ';' || strlen(line) == 0) {
            /* comment - skip */
            continue;
        }

        if (is_consecutive(line, ',')) {
            printf("Error: Multiple commas in line (%d).\n", line_number);
            is_code_with_errors = 1;
            continue;
        }

        /* check trailing commas */
        if (strlen(line) > 0 && line[strlen(line) - 1] == ',') {
            printf("Error: comma at the end of line (%d).\n", line_number);
            is_code_with_errors = 1;
            continue;
        }

        if (strchr(line, ':')) {
            is_line_with_label = 1;
            get_label(line, label);  /* can be wrong label so raise error */
            if (!is_valid_label(label)) {
                printf("Error: Invalid label (%s) encountered.\n", label);
                is_code_with_errors = 1;
                continue;
            }
        } else {
            is_line_with_label = 0;
        }

        mod_line = line;
        if (is_line_with_label) {
            mod_line += strlen(label) + 1;  /* now line is a command without label (+1 for :)*/
        }
        strip_whitespace(mod_line);

        if (is_line_with_label && is_label_exist(label, label_table, label_count)) {
            printf("Error: Label (%s) already exists.\n", label);
            is_code_with_errors = 1;
            continue;
        }

        if (is_data_instruction(mod_line) || is_string_instruction(mod_line)) {
            if (is_line_with_label) {
                last_error = add_label_to_symbol_table(&label_table, &label_count, label, DC, data_label);
                if (last_error) {
                    printf("Error: Couldn't add label (%s) to table.\n", label);
                    is_code_with_errors = 1;
                    continue;
                }
            }
            data_count_temp = data_count;
            if (is_data_instruction(mod_line)) {
                last_error = translate_data(&data, &data_count, mod_line);
            } else {
                last_error = translate_string(&data, &data_count, mod_line);
            }
            if (last_error) {
                printf("Error: Couldn't translate data/string. Line number (%d)\n", line_number);
                is_code_with_errors = 1;
                continue;
            }
            DC += (data_count - data_count_temp);
        }

        else if (is_entry_instruction(mod_line)) {
            continue;
        } 
        else if (is_extern_instruction(mod_line)) {
            token = strtok(mod_line, " \t"); /* Tokenize by space or tab */
            if (!token || strcmp(token, ".extern")) {
                printf("Error: Invalid extern line. Line number (%d)\n", line_number);
                is_code_with_errors = 1;
                continue;
            }
            token = strtok(NULL, " \t"); /* Get the next token, which is the name */
            if (is_reserved_word(token)) {
                printf("Error: Invalid extern label (%s) encountered.\n", token);
                is_code_with_errors = 1;
                continue;
            }

            last_error = add_label_to_symbol_table(&label_table, &label_count, token, IC, extern_label);
            if (last_error) {
                printf("Error: Couldn't add label (%s) to symbol table.\n", token);
                is_code_with_errors = 1;
                continue;
            }
        }
        else {
            /* this is an instruction! */
            if (is_line_with_label) {
                last_error = add_label_to_symbol_table(&label_table, &label_count, label, IC, code_label);
                if (last_error) {
                    printf("Error: Couldn't add label (%s) to symbol table.\n", label);
                    is_code_with_errors = 1;
                    continue;
                }
            }
            parse_instruction(&ins, mod_line);
            last_error = validate_instruction(&ins);
            if (last_error) {
                printf("Error: Couldn't validate instruction (%s) linu number (%d).\n", line, line_number);
                is_code_with_errors = 1;
                continue;
            }

            L = calculate_number_of_words(&ins);
            if (L == 1) {
                code[code_count].operand_code = NULL;    
            } else {
                code[code_count].operand_code = (operand*)malloc(sizeof(operand) * (L-1));
            }
            code[code_count].IC = IC;
            code[code_count].L = L;
            amount_opernads_resolved = build_instruction(&ins, &code[code_count]);  /* build all the immediate vals */
            code[code_count].need_to_resolve = amount_opernads_resolved != (L - 1);
            IC += L;
            code_count++;
        }
    }

    if (!is_code_with_errors) {
        ICF = IC;
        DCF = DC;
        for (i = 0; i < label_count; i++)
        {
            if (label_table[i].label_type == data_label) {
                label_table[i].address += ICF;
            }
        }
        
        last_error = second_cycle(file, label_table, label_count, code, code_count, &externals, &externals_count);
        if (!last_error) {
            save_obj_file(filename, code, code_count, data, data_count, ICF, DCF);
            save_entries_file(filename, label_table, label_count);
            save_externals_file(filename, externals, externals_count);
        }
    }

    for (i = 0; i < label_count; i++)
    {
        free(label_table[i].label_name);
    }
    free(label_table);
    free(data);
    for (i = 0; i < code_count; i++)
    {
        if (code[i].operand_code != NULL) {
            free(code[i].operand_code);
        }
    }
    for (i = 0; i < externals_count; i++)
    {
        free(externals[i].label_name);
    }
    free(externals);
}

void assemble(char* filename) {
    first_cycle(filename);
}