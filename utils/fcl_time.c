#include "fcl_time.h"

double get_diff_in_microseconds(struct timeval start) {
    struct timeval end;
    double time_difference;
    time_difference = 0;

    gettimeofday(&end, NULL);
    time_difference = difftime(end.tv_sec, start.tv_sec) * 1000;
    if (time_difference < 0) time_difference = 0;
    time_difference = time_difference + ((end.tv_usec - start.tv_usec) / 1000.0);
    if (time_difference < 0) time_difference = 0;

    return time_difference;
}
