#include "fcl_random.h"
#include <sys/time.h>
#include <stdlib.h>

void initialize_random_generator(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    srand((t.tv_sec * 1000) + (t.tv_usec / 1000));
}
