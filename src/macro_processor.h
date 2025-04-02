#pragma once

/**
 * Processes a single file, expanding macros and writing the result to an output file.
 * 
 * @param input_as_file The path to the input file with macros.
 * @return 0 on success, non-zero on error (e.g., file operation failure).
 */
int macro_process_file(const char* input_file);
