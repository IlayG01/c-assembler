#pragma once

#include <stdio.h>

#include "data_structs.h"


void assemble(char* filename);
void first_cycle(char* filename);
int second_cycle(FILE* file, label_element* label_table, size_t label_count, machine_code* code, size_t code_count, external_info* externals, size_t* externals_count);
