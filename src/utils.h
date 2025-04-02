#pragma once

#include <stdlib.h>


/**
 * Copies the filename from the source filename into the target filename with a different extension.
 * 
 * @param source_filename The original filename.
 * @param target_filename The buffer to store the new filename.
 * @param extension The new extension to append to the filename.
 */
void copy_filename_with_different_extension(const char* source_filename, char* target_filename, const char* extension);

/**
 * Removes leading and trailing whitespace from a string.
 * 
 * @param str The string to be stripped of whitespace. The operation is performed in-place.
 */
void strip_whitespace(char *str);

/**
 * Checks if a character appears consecutively in a string, ignoring spaces in between.
 * 
 * @param str The string to search.
 * @param search_str The character to look for.
 * @return 1 if the character appears consecutively, 0 otherwise.
 */
int is_consecutive(char* str, char search_str);

/**
 * Checks if a given name is a reserved word.
 * 
 * @param name The name to check.
 * @return 1 if the name is a reserved word, 0 otherwise.
 */
int is_reserved_word(const char* name);

/**
 * Checks if a given label is valid according to assembler rules.
 * 
 * @param array The array to extend.
 * @param current_size The current size of the array.
 * @param new_size The new size of the array.
 * @param element_size The size of each element in the array.
 * @return 0 on success, 1 on failure.
 */
int extend_array(void** array, size_t* current_size, size_t new_size, size_t element_size);