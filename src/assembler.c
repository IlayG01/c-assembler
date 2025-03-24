#include "assembler.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "consts.h"

#define MAX_BUF_SIZE 100
#define MAX_INSTRUCTIONS 1000
#define MAX_LABEL_LENGTH 31
#define MAX_OPERANDS 2


int assemble(char* filename) {
    int result;
    FILE *file = fopen(filename, "r");
    if (!file) {
        return FILE_NOT_EXIST;
    }

    result = first_cycle(file);
    if (SUCCESS != result) {
        return result;
    }
    /* rewind(file); */
    /* result = second_cycle(file); */
    /* if (SUCCESS != result) { */
    /*     return result; */
    /* } */
}

typedef enum {
    MOV = 0, CMP, ADD, SUB = 2, LEA = 4, CLR, NOT = 5, INC = 5, DEC = 5,
    JMP = 9, BNE = 9, JSR = 9, RED = 12, PRN, RTS, STOP, INVALID
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
    char* operands[MAX_OPERANDS];
} instruction;

typedef struct {
    size_t L;  /* 1/2/3 for code, othwise can be more */
    size_t IC; /* for external file */
    int need_to_resolve;  /* to know for second round if needed to be resolved. */
    first_word first_word_val;
    operand* operand_code;
} machine_code;

typedef enum {
    data_label,
    entry_label,
    extern_label,
    code_label
} label_options;

typedef struct {
    int address;
    char *label_name;
    /* int assembly_line; */
    label_options label_type;
} label_element;

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
    /* char label[MAX_LABEL_LENGTH + 1]; */
    strncpy(label, line, label_length);
    label[label_length] = '\0';

    /* Validate and print the label */
    /* if (is_valid_label(label)) { */
    /*     printf("Valid label: %s\n", label); */
    /* } else { */
    /*     printf("Invalid label\n"); */
    /* } */

    /* return 0; */
}

int is_label_exist(char* label, label_element* label_table, size_t label_count) {
    int i;
    for (i = 0; i < label_count; i++)
    {
        if (strcmp(label_table[i].label_name, label)) {
            return 1;
        }
    }
    
    return 0;
}

int is_data_instruction(char* ins) {
    return strchr(ins, '.data');
}

int is_entry_instruction(char* ins) {
    return strchr(ins, '.entry');
}

int is_extern_instruction(char* ins) {
    return strchr(ins, '.extern');
}

int is_string_instruction(char* ins) {
    return strchr(ins, '.string');
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
    (*label_table)[current_label_count].label_name = strdup(label);
    if (!(*label_table)[current_label_count].label_name) {
        return MEMORY_ALLOCATION_FAILED;
    }

    current_label_count++;
    return SUCCESS; /* Success */
}

int translate_data(data* data, size_t* count, char* line) {
    char *token = strtok(line, " ,"); /* Tokenize by spaces and commas */
    if (!token || strcmp(token, ".data") != 0) return -1; /* Ensure it's a `.data` directive */

    while ((token = strtok(NULL, " ,")) != NULL) {
        int value = atoi(token); /* Convert the token into an integer */

        data[*count].integer = value; /* Use the `data` struct's integer field */

        (*count)++;
    }
    return 0; /* Success */
}

int translate_string(data* data, size_t* count, char* line) {
    char *token = strtok(line, " ,"); /* Tokenize by spaces and commas */
    if (!token || strcmp(token, ".string") != 0) return 1; /* Ensure it's a `.string` directive */

    token = strtok(NULL, "\""); /* Get the string inside quotes */
    /* if (!token) return -1; /* Ensure a valid string is present */

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
        if (strcmp(str, OPCODE_STRINGS[i]) == 0) {
            return (opcode)i;
        }
    }
    return INVALID;  /* Return INVALID if not found */
}

instruction parse_instruction(const char* line) {
    instruction instr;
    instr.num_of_operands = 0;
    instr.operands[0] = NULL;
    instr.operands[1] = NULL;

    /* char buffer[MAX_BUF_SIZE]; */
    /* strcpy(buffer, line);  /* Copy to modify safely */
    char* token = strtok(line, " ,");  /* First token (opcode) */

    if (!token) {
        instr.opcode = INVALID;
        return instr;
    }

    instr.opcode = get_opcode(token);
    if (instr.opcode == INVALID) {
        return instr;
    }

    /* Extract operands */
    int i = 0;
    while ((token = strtok(NULL, " ,")) && i < MAX_OPERANDS) {
        instr.operands[i] = token;  
        instr.num_of_operands++;
        i++;
    }

    if (strtok(NULL, " ,") != NULL) {
        instr.opcode = INVALID;  /* Mark as invalid */
    }

    return instr;
}

/* Function to free allocated memory */
void free_instruction(instruction* instr) {
    int i;
    for (i = 0; i < instr->num_of_operands; i++) {
        free(instr->operands[i]);  /* Free dynamically allocated memory */
    }
}

typedef struct {
    opcode opcode;
    int funct;
    int num_of_operands;
    int valid_source_modes[3];  /* Allowed source operand addressing modes */
    int num_source_modes;
    int valid_dest_modes[3];    /* Allowed destination operand addressing modes */
    int num_dest_modes;
} OpcodeRule;

const OpcodeRule OPCODE_TABLE[] = {
    {MOV, 0, 2, {0, 1, 3}, 3, {1, 3}, 2},    /* mov */
    {CMP, 1, 2, {0, 1, 3}, 3, {0, 1, 3}, 3}, /* cmp */
    {ADD, 1, 2, {1, 3}, 2, {1, 3}, 2},       /* add */
    {SUB, 2, 2, {1, 3}, 2, {1, 3}, 2},       /* sub */
    {LEA, 4, 2, {1}, 1, {1, 3}, 2},          /* lea */
    {CLR, 1, 1, {}, 0, {1, 3}, 2},           /* clr */
    {NOT, 2, 1, {}, 0, {1, 3}, 2},           /* not */
    {INC, 3, 1, {}, 0, {1, 3}, 2},           /* inc */
    {DEC, 4, 1, {}, 0, {1, 3}, 2},           /* dec */
    {JMP, 1, 1, {}, 0, {1, 2}, 2},           /* jmp */
    {BNE, 2, 1, {}, 0, {1, 2}, 2},           /* bne */
    {JSR, 3, 1, {}, 0, {1, 2}, 2},           /* jsr */
    {RED, 0, 1, {}, 0, {1, 3}, 2},           /* red */
    {PRN, 0, 1, {}, 0, {0, 1, 3}, 3},        /* prn */
    {RTS, 0, 0, {}, 0, {}, 0},               /* rts */
    {STOP, 0, 0, {}, 0, {}, 0}               /* stop */
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

first_word generate_first_word(instruction* instr) {
    first_word first_word_val;
    first_word_val.A = 1;
    first_word_val.R = 0;
    first_word_val.E = 0;
    const OpcodeRule* opcode_rule = get_opcode_rule(instr->opcode);
    first_word_val.opcode_value = opcode_rule->opcode;
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
        first_word_val.dest_address = get_addressing_mode(instr->operands[1]);
        if (first_word_val.dest_address == 3) {
            first_word_val.dest_reg = instr->operands[1][1] - '0';
        }
    } else {
        first_word_val.src_address = 0;
        first_word_val.src_reg = 0;
        first_word_val.dest_address = 0;
        first_word_val.dest_reg = 0;
    }

    return first_word_val;
}

int calculate_number_of_words(instruction* instr) {
    if (instr->num_of_operands == 2) {
        /* Instruction has both source and destination operands */
        int src_mode = get_addressing_mode(instr->operands[0]);
        int dest_mode = get_addressing_mode(instr->operands[1]);

        return 1 + (src_mode != 3) + (dest_mode != 3);
    } else if (instr->num_of_operands == 1) {
        /* Instruction only has a destination operand */
        int dest_mode = get_addressing_mode(instr->operands[0]);
        return 1 + dest_mode != 3;
    }
    return 1;
}

operand generate_operand_code(char* operand_val) {
    operand operandd = {0};
    operandd.A = 1;
    operandd.R = 0;
    operandd.E = 0;
    operandd.integer = atoi(operand_val+1); /* skip the # */
    return operandd;
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

int first_cycle(FILE* file) {
    char line[MAX_BUF_SIZE];  /* Line Max Size = 80 */
    int len;
    int error;
    size_t IC = 100;
    size_t DC = 0;
    machine_code code[MAX_INSTRUCTIONS];
    data data[MAX_INSTRUCTIONS]; 
    label_element* label_table = NULL;
    size_t label_count = 0;
    size_t data_count = 0;
    size_t code_count = 0;
    /* TODO: add error detection */
    while (len = fgets(line, sizeof(line), file)) {
        if (len > 80) {
            return LINE_TOO_LONG;
        }
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
        char* mod_line = line + strlen(label);  /* now line is a command without label */

        if (is_line_with_label && is_label_exist(label, label_table, label_count)) {
            return LABEL_ALREADY_EXIST;
        }

        if (is_data_instruction(mod_line) || is_string_instruction(mod_line)) {
            if (is_line_with_label) {
                error = add_label_to_symbol_table(&label_table, &label_count, label, DC, data_label);
                if (!error) {
                    return error;
                }
            }
            size_t data_count_temp = data_count;
            if (is_data_instruction(mod_line)) {
                error = translate_data(data, &data_count, mod_line);
            } else {
                error = translate_string(data, &data_count, mod_line);
            }
            if (!error) {
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
            if (!error) {
                return error;
            }
        }
        else {
            /* this is an instruction! */
            if (is_line_with_label) {
                error = add_label_to_symbol_table(&label_table, &label_count, label, IC, code_label);
                if (!error) {
                    return error;
                }
            }
            instruction ins = parse_instruction(mod_line);
            error = validate_instruction(&ins);
            if (!error) {
                return error;
            }

            int L = calculate_number_of_words(mod_line);
            if (L == 1) {
                code[code_count].operand_code = NULL;    
            } else {
                code[code_count].operand_code = (operand*)malloc(sizeof(operand) * (L-1));
            }
            code->IC = IC;
            code->L = L;
            int resolved = 0;
            if (L > 1) {
                resolved = build_instruction(&ins, &code[code_count]);  /* build all the immediate vals */
            }
            code->need_to_resolve = resolved != (L - 1);
            IC += L;
            code_count++;
        }
        /* need to think how to order the binary machine code (when iterating over the code) */
            /* need to consider that there might be a opcodes which we cannot convert immediately only at round 2 */
            /* struct of command might not be a good impl because there are lines like .data that need to be convert and are not command. */
        /* a solution might be translating anything we can, if we cannot because it contains LABEL, leave it blank and at the second round */
        /* fill it in order of the symbol table. can be a good idea to save the Length of the commands to not translate it again. */
        /* TODO: how to translate assembly to binary data, add data structs accordingly to what written in the .docx */

        /* if comment continue */
        /* if הנחיה so  */
            /* .data - allocate in the data image and increment DC */
                /* if there is a label so the label gets DC before inc and inserted to the symbol table */
            /* .string - allocate in the data image, each char gets a while word */
                /* add \0 in the end of the string */
                /* if there is a label , same as .data */
            /* .entry - skip */
            /* .extern - add to symbol table with the value 0 and continue */
                /* ignore label */
        /* if הוראה so */
          /* create a table of symbols contains the symbol, his value and properties (ignore entry - take care in second round) */
          /* try to create a struct of the command - if there is a label leave it to the next round */
          /* estimate the size of the command and append the IC */
          /* ARE also set at the next round */
        /* think how to handle multiple errors (prabably a linked list of errors - line number, error, maybe params)  */
        /* add a pretty print for errors */
    }
    int i;
    for (i = 0; i < label_count; i++)
    {
        free(label_table->label_name);
    }
    free(label_table);
    for (i = 0; i < MAX_INSTRUCTIONS; i++)
    {
        if (code->operand_code != NULL) {
            free(code->operand_code);
        }
    }
    

    /* values are used for building object file */
    /* ICF = IC; */
    /* DCF = DC; */
    /* for symbol in symbol_table: */
    /*     if data: */
    /*         symbol_addr += ICF; */

    /* TODO: understand how to save the index of the labal to resolve it easily in round 2     */


    /* second cycle */
    /* write to files */
    /* errors */
    /* run this code */
    /* free memory */
    /* split to files */
}

int second_cycle(FILE* file) {
/* if this is entry command look for the symbol table */
/* iterate over the structs of commands. complete them if necessary (in case of a label) */
/* create the actual bytes */
/* create the files for each  */
}
