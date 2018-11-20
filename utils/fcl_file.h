#ifndef FCL_FILE_H
#define FCL_FILE_H

#include "types.h"

/**
 * Check if a file exists on disk!
 *
 * @param[in] fname Filepath which shall be checked.
 * @return
 */
uint32_t exists(const char *fname);

/**
 * Read uint64 array comma separated to disk
 *
 * @param[in] fname Filepath of the source file.
 * @param[out] no_elements Number of elements of the array.
 * @param[out] elements Pointer to the array.
 * @return
 */
void read_uint64_array_from_file(const char *fname,
                                 uint64_t* no_elements,
                                 uint64_t** elements);

/**
 * Write uint64 array comma separated to disk
 *
 * @param[in] fname Filepath of the destination file.
 * @param[in] no_elements Number of elements of the array
 * @param[in] elements Pointer to the array.
 * @return
 */
void write_uint64_array_from_file(const char *fname,
                                  uint64_t no_elements,
                                  uint64_t* elements);

#endif /* FCL_FILE_H */
