#pragma once

int macro_process_file(const char* input_file);
int is_valid_macro_name(const char* name);
int is_reserved_word(const char* name);
int find_macro(const char* name);
void trim_whitespace(char* str);
void strip_newline(char* str);
void add_macro(const char* name);
void add_line_to_macro(int macro_index, const char* line);
void initialize_macro_table();
