#ifndef FCL_FILE_H
#define FCL_FILE_H

#include "types.h"
#include <stdio.h>

/**
 * Check if a file exists on disk!
 *
 * @param[in] fname Filepath which shall be checked.
 * @return
 */
uint32_t exists(const char *fname);

/**
 * Dump a uint64 array as json to an open file
 *
 * @param[in] file The open file pointer
 * @param[in] indent The indent to usi in the file for this entry
 * @param[in] array_name The name of the array
 * @param[in] no_elements Number of elements in elements array
 * @param[in] elements Array that holds the elements that shall be written
 * @param[in] with_ending_comma Boolean value, if >0 write a, at the end of the line
 * @return
 */
void json_dump_uint64_array_to_open_file(FILE *file,
                                         uint32_t indent,
                                         char* array_name,
                                         uint64_t no_elements,
                                         uint64_t* elements,
                                         uint32_t with_ending_comma);

#endif /* FCL_FILE_H */
