#ifndef FCL_RANDOM_H
#define FCL_RANDOM_H

#include "types.h"
#include <stdlib.h>

#ifndef __USE_POSIX
/* rand_r is not in the ansi standard */
int rand_r(uint32_t * seedptr);
#endif

/**
 * @brief Initialize random generator with microsecond precision.
 */
void initialize_random_generator(void);

#endif /* RANDOM_H */
