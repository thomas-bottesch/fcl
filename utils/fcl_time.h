#ifndef FCL_TIME_H
#define FCL_TIME_H

#include <sys/time.h>
#include <time.h>

/**
 * @Brief Get time difference in microseconds between now and a given time.
 *
 * @param[in] start
 * @return Time difference in microseconds.
 */
double get_diff_in_microseconds(struct timeval start);

#endif /* FCL_TIME_H */
