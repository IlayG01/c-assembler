#include "consts.h"


/* Mapping opcodes from strings to enum */
const char* OPCODE_STRINGS[] = {
    "mov", "cmp", "add", "sub", "lea",
    "clr", "not", "inc", "dec", "jmp",
    "bne", "jsr", "red", "prn", "rts", "stop"
};


/* Array of opcode rules defining the behavior and constraints of each opcode.
 Each entry specifies the opcode, its function, allowed operands, and addressing modes.
*/
const OpcodeRule OPCODE_TABLE[] = {
    {MOV, 0, 0, 2, {0, 1, 3}, 3, {1, 3}, 2},    
    {CMP, 1, 0, 2, {0, 1, 3}, 3, {0, 1, 3}, 3}, 
    {ADD, 2, 1, 2, {0, 1, 3}, 3, {1, 3}, 2},       
    {SUB, 2, 2, 2, {0, 1, 3}, 3, {1, 3}, 2},       
    {LEA, 4, 0, 2, {1}, 1, {1, 3}, 2},          
    {CLR, 5, 1, 1, {0}, 0, {1, 3}, 2},           
    {NOT, 5, 2, 1, {0}, 0, {1, 3}, 2},           
    {INC, 5, 3, 1, {0}, 0, {1, 3}, 2},           
    {DEC, 5, 4, 1, {0}, 0, {1, 3}, 2},           
    {JMP, 9, 1, 1, {0}, 0, {1, 2}, 2},           
    {BNE, 9, 2, 1, {0}, 0, {1, 2}, 2},           
    {JSR, 9, 3, 1, {0}, 0, {1, 2}, 2},           
    {RED, 12, 0, 1, {0}, 0, {1, 3}, 2},           
    {PRN, 13, 0, 1, {0}, 0, {0, 1, 3}, 3},        
    {RTS, 14, 0, 0, {0}, 0, {0}, 0},               
    {STOP, 15, 0, 0, {0}, 0, {0}, 0}               
};

/* The size of the OPCODE_TABLE array, used for iteration, validation, and lookup operations. */
const int OPCODE_TABLE_SIZE = sizeof(OPCODE_TABLE) / sizeof(OPCODE_TABLE[0]);
