#include "assembler.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "utils.h"
#include "consts.h"

#define MAX_BUF_SIZE 100
#define MAX_INSTRUCTIONS 1000
#define MAX_LABEL_LENGTH 31
#define MAX_OPERANDS 2
#define WORD_SIZE 3  /* 24/8 bytes*/


int assemble(char* filename) {
    int result;
    printf("assemble: %s", filename);
    result = first_cycle(filename);
    return result;
}

typedef enum {
    MOV, CMP, ADD, SUB, LEA, CLR, NOT, INC, DEC,
    JMP, BNE, JSR, RED, PRN, RTS, STOP, INVALID
} opcode;

/* Mapping opcodes from strings to enum */
const char* OPCODE_STRINGS[] = {
    "mov", "cmp", "add", "sub", "lea",
    "clr", "not", "inc", "dec", "jmp",
    "bne", "jsr", "red", "prn", "rts", "stop"
};


typedef struct {
    unsigned int E: 1;  /* always 0 */
    unsigned int R: 1;  /* always 0 */
    unsigned int A: 1;  /* always 1 */
    unsigned int funct: 5;
    unsigned int dest_reg: 3;
    unsigned int dest_address: 2;
    unsigned int src_reg: 3;
    unsigned int src_address: 2;
    unsigned int opcode_value: 6;
} first_word;


typedef struct {
    unsigned int E: 1;
    unsigned int R: 1;
    unsigned int A: 1;
    unsigned int integer: 21;
} operand;

typedef struct {
    union {
        int integer: 24;
        unsigned int ascii: 24;
    };
} data;

typedef struct {
    opcode opcode;
    size_t num_of_operands;
    char operands[MAX_OPERANDS][MAX_LABEL_LENGTH];
} instruction;

typedef struct {
    size_t L;  /* 1/2/3 for code */
    size_t IC; /* for external file */
    int need_to_resolve;  /* to know for second round if needed to be resolved. */
    first_word first_word_val;
    operand* operand_code;
} machine_code;

typedef enum {
    data_label = 0x1,
    entry_label = 0x10,
    extern_label = 0x100,
    code_label = 0x1000
} label_options;

typedef struct {
    int address;
    char *label_name;
    /* int assembly_line; */
    label_options label_type;
} label_element;

typedef struct {
    int address;
    char *label_name;
} external_info;

/* Function to check if a label is valid */
int is_valid_label(const char *label) {
    int len = strlen(label);

    /* Check length constraint */
    if (len == 0 || len > MAX_LABEL_LENGTH) {
        return 0;
    }

    /* Check first character (must be a letter) */
    if (!isalpha(label[0])) {
        return 0;
    }

    /* Check remaining characters (must be alphanumeric) */
    int i;
    for (i = 1; i < len; i++) {
        if (!isalnum(label[i])) {
            return 0;
        }
    }

    return 1;
}

void get_label(char* line, char* label) {
    /* Find the position of ':' */
    char *colon_pos = strchr(line, ':');

    /* Extract the label */
    int label_length = colon_pos - line;
    
    /* Copy the label to a separate string and remove trailing spaces */
    strncpy(label, line, label_length);
    label[label_length] = '\0';
}

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

int is_data_instruction(char* ins) {
    return NULL != strstr(ins, ".data");
}

int is_entry_instruction(char* ins) {
    return NULL != strstr(ins, ".entry");
}

int is_extern_instruction(char* ins) {
    return NULL != strstr(ins, ".extern");
}

int is_string_instruction(char* ins) {
    return NULL != strstr(ins, ".string");
}

int add_label_to_symbol_table(label_element** label_table, size_t* label_count, char* label, size_t address, label_options label_type) {
    size_t current_label_count = *label_count;
    /* Allocate memory for the new label table (increase the size) */
    label_element* new_table = realloc(*label_table, (current_label_count + 1) * sizeof(label_element));
    if (!new_table) {
        return MEMORY_ALLOCATION_FAILED;
    }

    /* Update the table pointer */
    *label_table = new_table;

    /* Assign values to the new label entry */
    (*label_table)[current_label_count].address = address;
    (*label_table)[current_label_count].label_type = label_type;

    /* Allocate memory for the label name and copy it */
    char* label_copy = (char*)malloc(strlen((label)) + 1);
    if (!label_copy) {
        return MEMORY_ALLOCATION_FAILED;
    }
    strcpy(label_copy, label);

    (*label_table)[current_label_count].label_name = label_copy;

    (*label_count)++;
    return SUCCESS; /* Success */
}

int translate_data(data* data, size_t* count, char* line) {
    char *token = strtok(line, " ,"); /* Tokenize by spaces and commas */
    if (!token || strcmp(token, ".data")) return -1; /* Ensure it's a `.data` directive */

    while ((token = strtok(NULL, " ,")) != NULL) {
        int value = atoi(token); /* Convert the token into an integer */

        data[*count].integer = value; /* Use the `data` struct's integer field */
        (*count)++;
    }
    return 0; /* Success */
}

int translate_string(data* data, size_t* count, char* line) {
    char *token = strtok(line, " ,"); /* Tokenize by spaces and commas */
    if (!token || strcmp(token, ".string")) return 1; /* Ensure it's a `.string` directive */

    token = strtok(NULL, "\""); /* Get the string inside quotes */

    size_t str_len = strlen(token);

    size_t i;
    for (i = 0; i < str_len; i++) {
        data[*count].ascii = (int)token[i]; /* Use the `data` struct's integer field */
        (*count)++;
    }
    data[*count].ascii = 0; /* null */
    (*count)++;

    return 0; /* Success */
}

opcode get_opcode(const char* str) {
    int i;
    for (i = 0; i < INVALID; i++) {
        if (!strcmp(str, OPCODE_STRINGS[i])) {
            return (opcode)i;
        }
    }
    return INVALID;  /* Return INVALID if not found */
}

void parse_instruction(instruction* instr, const char* line) {
    instr->num_of_operands = 0;

    char buffer[MAX_BUF_SIZE];
    strcpy(buffer, line);  /* Copy to modify safely */
    char* token = strtok(buffer, " ,");  /* First token (opcode) */
    

    if (!token) {
        instr->opcode = INVALID;
        return;
    }

    instr->opcode = get_opcode(token);
    if (instr->opcode == INVALID) {
        return;
    }

    /* Extract operands */
    int i = 0;
    while ((token = strtok(NULL, " ,")) && i < MAX_OPERANDS) {
        strcpy(instr->operands[i], token);
        instr->num_of_operands++;
        i++;
    }

    if (strtok(NULL, " ,") != NULL) {
        instr->opcode = INVALID;  /* Mark as invalid */
    }
}

typedef struct {
    opcode opcode;
    int opcode_value;
    int funct;
    int num_of_operands;
    int valid_source_modes[3];  /* Allowed source operand addressing modes */
    int num_source_modes;
    int valid_dest_modes[3];    /* Allowed destination operand addressing modes */
    int num_dest_modes;
} OpcodeRule;

const OpcodeRule OPCODE_TABLE[] = {
    {MOV, 0, 0, 2, {0, 1, 3}, 3, {1, 3}, 2},    
    {CMP, 1, 0, 2, {0, 1, 3}, 3, {0, 1, 3}, 3}, 
    {ADD, 2, 1, 2, {1, 3}, 2, {1, 3}, 2},       
    {SUB, 2, 2, 2, {1, 3}, 2, {1, 3}, 2},       
    {LEA, 4, 0, 2, {1}, 1, {1, 3}, 2},          
    {CLR, 5, 1, 1, {}, 0, {1, 3}, 2},           
    {NOT, 5, 2, 1, {}, 0, {1, 3}, 2},           
    {INC, 5, 3, 1, {}, 0, {1, 3}, 2},           
    {DEC, 5, 4, 1, {}, 0, {1, 3}, 2},           
    {JMP, 9, 1, 1, {}, 0, {1, 2}, 2},           
    {BNE, 9, 2, 1, {}, 0, {1, 2}, 2},           
    {JSR, 9, 3, 1, {}, 0, {1, 2}, 2},           
    {RED, 12, 0, 1, {}, 0, {1, 3}, 2},           
    {PRN, 13, 0, 1, {}, 0, {0, 1, 3}, 3},        
    {RTS, 14, 0, 0, {}, 0, {}, 0},               
    {STOP, 15, 0, 0, {}, 0, {}, 0}               
};

const int OPCODE_TABLE_SIZE = sizeof(OPCODE_TABLE) / sizeof(OPCODE_TABLE[0]);

/* Identify the addressing mode of an operand */
int get_addressing_mode(const char* operand) {
    if (!operand) return -1;
    if (operand[0] == '#') return 0;  /* Immediate */
    if (operand[0] == '&') return 2;  /* Relative */
    if (operand[0] == 'r' && strlen(operand) == 2 && operand[1] >= '0' && operand[1] <= '7') return 3; /* Register */
    return 1;  /* Direct */
}

/* Check if mode is allowed for an operand */
int is_mode_allowed(int mode, const int allowed_modes[], int num_modes) {
    int i;
    for (i = 0; i < num_modes; i++) {
        if (allowed_modes[i] == mode) return 1;
    }
    return 0;
}

const OpcodeRule* get_opcode_rule(opcode opcode) {
    int i;
    for (i = 0; i < OPCODE_TABLE_SIZE; i++) {
        if (OPCODE_TABLE[i].opcode == opcode) {
            return &OPCODE_TABLE[i];  /* Return pointer to found rule */
        }
    }
    return NULL;  /* Return NULL if not found */
}

int validate_instruction(const instruction* instr) {
    /* if (!instr) return 0; */

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

first_word generate_first_word(const instruction* instr) {
    first_word first_word_val;
    first_word_val.A = 1;
    first_word_val.R = 0;
    first_word_val.E = 0;
    const OpcodeRule* opcode_rule = get_opcode_rule(instr->opcode);
    first_word_val.opcode_value = opcode_rule->opcode_value;
    first_word_val.funct = opcode_rule->funct;
    if (instr->num_of_operands == 2) {
        first_word_val.src_address = get_addressing_mode(instr->operands[0]);
        if (first_word_val.src_address == 3) {
            first_word_val.src_reg = instr->operands[0][1] - '0';
        }
        first_word_val.dest_address = get_addressing_mode(instr->operands[1]);
        if (first_word_val.dest_address == 3) {
            first_word_val.dest_reg = instr->operands[1][1] - '0';
        }
    } else if (instr->num_of_operands == 1) {
        first_word_val.src_address = 0;
        first_word_val.src_reg = 0;
        first_word_val.dest_address = get_addressing_mode(instr->operands[0]);
        if (first_word_val.dest_address == 3) {
            first_word_val.dest_reg = instr->operands[0][1] - '0';
        }
    } else {
        first_word_val.src_address = 0;
        first_word_val.src_reg = 0;
        first_word_val.dest_address = 0;
        first_word_val.dest_reg = 0;
    }

    return first_word_val;
}

int calculate_number_of_words(const instruction* instr) {
    if (instr->num_of_operands == 2) {
        /* Instruction has both source and destination operands */
        int src_mode = get_addressing_mode(instr->operands[0]);
        int dest_mode = get_addressing_mode(instr->operands[1]);

        return 1 + (src_mode != 3) + (dest_mode != 3);
    } else if (instr->num_of_operands == 1) {
        /* Instruction only has a destination operand */
        int dest_mode = get_addressing_mode(instr->operands[0]);
        return 1 + (dest_mode != 3);
    }
    return 1;
}

operand generate_operand_code(char* operand_val) {
    operand operand = {0};
    operand.A = 1;
    operand.R = 0;
    operand.E = 0;
    operand.integer = atoi(operand_val+1); /* skip the # */
    return operand;
}

int build_instruction(instruction* instr, machine_code* machine_code) {
    int i;
    int resolved = 0;
    int operand_code_index = 0;
    int address_mode;

    machine_code->first_word_val = generate_first_word(instr);
    for (i = 0; i < instr->num_of_operands; i++) {
        address_mode = get_addressing_mode(instr->operands[i]);
        if (address_mode == 0) {
            resolved++;
            machine_code->operand_code[operand_code_index] = generate_operand_code(instr->operands[i]);
        }
        if (address_mode != 3) {
            operand_code_index++;  /* no additional word for reg address */
        }
    }
    return resolved;
}

void strip_whitespace(char *str) {
    int start = 0, end = strlen(str) - 1;

    /* Trim leading whitespace */
    while (isspace((unsigned char)str[start])) {
        start++;
    }

    /* Trim trailing whitespace */
    while (end > start && isspace((unsigned char)str[end])) {
        end--;
    }

    /* Shift characters forward */
    int i;
    for (i = 0; i <= end - start; i++) {
        str[i] = str[start + i];
    }
    str[i] = '\0'; /* Null-terminate the string */
}

void save_obj_file(const char* filename, machine_code* code, size_t code_count, data* data, size_t data_count, size_t ICF, size_t DCF) {
    char obj_filename[FILENAME_MAX];
    FILE* file = NULL;
    int line_number = 100;
    int i, j;

    replace_extension(filename, obj_filename, ".obj");
    printf("obj: %s", obj_filename);
    file = fopen(obj_filename, "w"); /* TODO: util function to open files safely */

    fprintf(file, "%7ld %ld\n", ICF-100, DCF);
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
        fprintf(file, "%07d %06X\n", line_number++, data[i].integer);
    }

    fclose(file);
}

void save_entries_file(const char* filename, label_element* label_table, size_t label_count) {
    char ent_filename[FILENAME_MAX];
    FILE* file = NULL;
    int i;

    replace_extension(filename, ent_filename, ".ent");
    printf("ent: %s", ent_filename);
    file = fopen(ent_filename, "w"); /* TODO: util function to open files safely */

    for (i = 0; i < label_count; i++) {
        if (label_table[i].label_type & entry_label) {
            fprintf(file, "%s %07d\n", label_table[i].label_name, label_table[i].address);
        }
    }

    fclose(file);
}

void save_externals_file(const char* filename, external_info* externals, size_t externals_count) {
    char ext_filename[FILENAME_MAX];
    FILE* file = NULL;
    int i;

    replace_extension(filename, ext_filename, ".ext");
    printf("ect: %s", ext_filename);
    file = fopen(ext_filename, "w"); /* TODO: util function to open files safely */

    for (i = 0; i < externals_count; i++) {
        fprintf(file, "%s %07d\n", externals[i].label_name, externals[i].address);
    }

    fclose(file);
}

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

void write_operand_hex_to_file(FILE* file, operand* operand) {
    unsigned int value = 0;

    value |= (operand->E        & 0x1)       << 0;   
    value |= (operand->R        & 0x1)       << 1;   
    value |= (operand->A        & 0x1)       << 2;   
    value |= (operand->integer  & 0x1FFFFF)  << 3;   

    fprintf(file, "%06X\n", value & 0xFFFFFF);
}

int first_cycle(char* filename) {
    char line[MAX_BUF_SIZE];  /* Line Max Size = 80 */
    int len;
    int error;
    size_t IC = 100;
    size_t DC = 0;
    machine_code code[MAX_INSTRUCTIONS];
    data data[MAX_INSTRUCTIONS];
    external_info externals[MAX_INSTRUCTIONS];
    label_element* label_table = NULL;
    size_t label_count = 0;
    size_t data_count = 0;
    size_t code_count = 0;
    size_t externals_count = 0;
    FILE *file = fopen(filename, "r");

    if (!file) {
        return FILE_NOT_EXIST;
    }
    
    /* TODO: add error detection */
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strlen(line) > 80) {
            return LINE_TOO_LONG;
        }
        strip_whitespace(line);

        if (line[0] == ';') {
            /* comment - skip */
            continue;
        }

        char label[MAX_LABEL_LENGTH + 1] = {0};
        int is_line_with_label = 0;
        if (strchr(line, ':')) {
            is_line_with_label = 1;
            get_label(line, label);  /* can be wrong label so raise error */
            if (!is_valid_label(label)) {
                return INVALID_LABEL;
            }
        }

        char* mod_line = line;
        if (is_line_with_label) {
            mod_line += strlen(label) + 1;  /* now line is a command without label (+1 for :)*/
        }
        strip_whitespace(mod_line);

        if (is_line_with_label && is_label_exist(label, label_table, label_count)) {
            return LABEL_ALREADY_EXIST;
        }

        if (is_data_instruction(mod_line) || is_string_instruction(mod_line)) {
            if (is_line_with_label) {
                error = add_label_to_symbol_table(&label_table, &label_count, label, DC, data_label);
                if (error) {
                    return error;
                }
            }
            size_t data_count_temp = data_count;
            if (is_data_instruction(mod_line)) {
                error = translate_data(data, &data_count, mod_line);
            } else {
                error = translate_string(data, &data_count, mod_line);
            }
            if (error) {
                return error;
            }
            DC += (data_count - data_count_temp);
        }

        else if (is_entry_instruction(mod_line)) {
            continue;
        } 
        else if (is_extern_instruction(mod_line)) {
            char* token = strtok(mod_line, " \t"); /* Tokenize by space or tab */
            token = strtok(NULL, " \t"); /* Get the next token, which is the name */
            error = add_label_to_symbol_table(&label_table, &label_count, token, IC, extern_label);
            if (error) {
                return error;
            }
        }
        else {
            /* this is an instruction! */
            if (is_line_with_label) {
                error = add_label_to_symbol_table(&label_table, &label_count, label, IC, code_label);
                if (error) {
                    return error;
                }
            }
            instruction ins;
            parse_instruction(&ins, mod_line);
            error = validate_instruction(&ins);
            if (error) {
                return error;
            }

            int L = calculate_number_of_words(&ins);
            if (L == 1) {
                code[code_count].operand_code = NULL;    
            } else {
                code[code_count].operand_code = (operand*)malloc(sizeof(operand) * (L-1));
            }
            code[code_count].IC = IC;
            code[code_count].L = L;
            int resolved = build_instruction(&ins, &code[code_count]);  /* build all the immediate vals */
            code[code_count].need_to_resolve = resolved != (L - 1);
            IC += L;
            code_count++;
        }
    }

    int i;
    size_t ICF;
    size_t DCF;
    ICF = IC;
    DCF = DC;
    for (i = 0; i < label_count; i++)
    {
        if (label_table[i].label_type == data_label) {
            label_table[i].address += ICF;
        }
    }
    

    second_cycle(file, label_table, label_count, code, code_count, externals, &externals_count);

    save_obj_file(filename, code, code_count, data, data_count, ICF, DCF);
    save_entries_file(filename, label_table, label_count);
    save_externals_file(filename, externals, externals_count);
    

    for (i = 0; i < label_count; i++)
    {
        free(label_table[i].label_name);
    }
    free(label_table);
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
    
    return SUCCESS;
}

int second_cycle(FILE* file, label_element* label_table, size_t label_count, machine_code* code, size_t code_count, external_info* externals, size_t* externals_count) {
    char line[MAX_BUF_SIZE];  /* Line Max Size = 80 */
    int code_line_number = 0;
    int len;
    int i, j;
    int error;

    rewind(file);
    while (len = fgets(line, sizeof(line), file)) {
        strip_whitespace(line);

        if (line[0] == ';') {
            /* comment - skip */
            continue;
        }

        char label[MAX_LABEL_LENGTH + 1] = {0};
        char* mod_line = line;
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
            token = strtok(NULL, " \t"); /* Get the next token, which is the name */

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
            return ENTRY_LABEL_DOESNT_EXIST;
        }

        if (code[code_line_number].need_to_resolve) {
            instruction instr;
            int address_mode;
            int operand_code_index = 0;
            int label_type;
            char* label_name;

            parse_instruction(&instr, mod_line);
            for (i = 0; i < instr.num_of_operands; i++) {
                address_mode = get_addressing_mode(instr.operands[i]);
                if (address_mode == 0) {
                    operand_code_index++;  /* already built */
                    continue;
                }
                if (address_mode == 3) {
                    continue;  /* no additional word for reg address */
                }

                label_name = instr.operands[i];
                if (address_mode == 2) {
                    /* may contain & as prefix */
                    label_name++;
                }
                if (!is_label_exist(label_name, label_table, label_count)) {
                    return LABEL_DOES_NOT_EXIST;
                }

                int label_address;
                for (j = 0; j < label_count; j++)
                {
                    if (!strcmp(label_name, label_table[j].label_name)) {
                        label_address = label_table[j].address;
                        label_type = label_table[j].label_type;
                        break;
                    }
                }

                if (label_type == extern_label) {
                    if (address_mode == 2) {
                        return INVALID_JUMP_TO_EXTERNAL_ADDRESS;
                    }

                    externals[*externals_count].address = code[code_line_number].IC + 1 + operand_code_index;
                    char* label_copy = (char*)malloc(strlen((label_name)) + 1);
                    if (!label_copy) {
                        return MEMORY_ALLOCATION_FAILED;
                    }
                    strcpy(label_copy, label_name);
                    externals[*externals_count].label_name = label_copy;
                    (*externals_count)++;

                    code[code_line_number].operand_code[operand_code_index].A = 0;
                    code[code_line_number].operand_code[operand_code_index].R = 0;
                    code[code_line_number].operand_code[operand_code_index].E = 1;
                    code[code_line_number].operand_code[operand_code_index].integer = 0;
                } else {
                    code[code_line_number].operand_code[operand_code_index].E = 0;
                    if (address_mode == 2) {
                        code[code_line_number].operand_code[operand_code_index].A = 1;                        
                        code[code_line_number].operand_code[operand_code_index].R = 0;
                        code[code_line_number].operand_code[operand_code_index].integer = label_address - code[code_line_number].IC;
                    } else {
                        /* address mode == 1 */
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
}
