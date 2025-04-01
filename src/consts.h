#pragma once

#include "data_structs.h"


enum ReturnCodes {
    SUCCESS = 0,
    NO_INPUT_FILES,
    MEMORY_ALLOCATION_FAILED,
    INVALID_OPCODE,
    WRONG_AMOUNT_OF_OPERANDS,
    INVALID_SRC_OPERAND_ADDRESSING_MODE,
    INVALID_DST_OPERAND_ADDRESSING_MODE,
};

extern const char* OPCODE_STRINGS[16];

extern const OpcodeRule OPCODE_TABLE[16];

extern const int OPCODE_TABLE_SIZE;
