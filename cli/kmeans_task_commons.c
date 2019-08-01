#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "kmeans_task_commons.h"

void add_additional_param_float(struct kmeans_params* prms, char* key_float_pair) {
    KEY_TYPE colon_position;
    char *endptr;
    char *float_value_as_str;
    VALUE_TYPE result;

    colon_position = strchr(key_float_pair, ':') - key_float_pair;
    key_float_pair[colon_position] = '\0';
    float_value_as_str = key_float_pair + colon_position + 1;
    errno = 0;
    result = strtod(float_value_as_str, &endptr);

    if (errno == ERANGE) {
        printf("Error: %s:%s float overflow (skipping param)\n"
               , key_float_pair
               , float_value_as_str);
    } else if (*endptr || errno == EINVAL) {
        printf("Error: %s:%s not a valid float (skipping param)\n"
               , key_float_pair
               , float_value_as_str);
    } else {
        d_add_subfloat(&(prms->tr)
                     , "additional_params"
                     , key_float_pair
                     , result);
    }
}

void add_additional_info_param(struct kmeans_params* prms, char* key_str_pair) {
    KEY_TYPE colon_position;
    char *str_value_as_str;

    colon_position = strchr(key_str_pair, ':') - key_str_pair;
    key_str_pair[colon_position] = '\0';
    str_value_as_str = key_str_pair + colon_position + 1;

    d_add_substring(&(prms->tr)
                 , "info"
                 , key_str_pair
                 , str_value_as_str);
}
