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

#endif /* FCL_FILE_H */
