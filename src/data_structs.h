
#pragma once

#include <stdlib.h>

#define MAX_LABEL_LENGTH 31
#define MAX_OPERANDS 2


typedef enum {
    MOV, CMP, ADD, SUB, LEA, CLR, NOT, INC, DEC,
    JMP, BNE, JSR, RED, PRN, RTS, STOP, INVALID
} opcode;

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
    } value;
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
    entry_label = 0x2,
    extern_label = 0x4,
    code_label = 0x8
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