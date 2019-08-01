#ifndef KMEANS_TASK_COMMONS_H
#define KMEANS_TASK_COMMONS_H

#include "../algorithms/kmeans/kmeans_control.h"

/**
 * @brief The command line task to start a kmeans clustering.
 *
 */
void add_additional_param_float(struct kmeans_params* prms, char* key_float_pair);
void add_additional_info_param(struct kmeans_params* prms, char* key_str_pair);

#endif