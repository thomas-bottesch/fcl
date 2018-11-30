#ifndef INIT_PARAMS_H
#define INIT_PARAMS_H

#include "../../utils/types.h"
#include <stdio.h>

#define TOKEN_INITIAL_CLUSTER_SAMPLES "initial_cluster_samples"
#define TOKEN_ASSIGNMENTS "assignments"

/**
 * @brief Parameters to control initialization step of kmeans
 */
struct initialization_params {
    uint64_t  len_assignments;              /**< this number must correspond with the number of samples which are clustered */
    uint64_t* assignments;                  /**< an array where the position corresonds to a sample and the content is a cluster */
    uint64_t  len_initial_cluster_samples;  /**< Length of the initial_cluster_samples array */
    uint64_t* initial_cluster_samples;      /**< A list of sample_ids that served as initial cluster centers */
};

void write_initialization_params_file(const char* fname,
                                      struct initialization_params* initprms);

void read_initialization_params_file(const char* fname,
                                     struct initialization_params** initprms);
                                     
void free_init_params(struct initialization_params* initprms);

#endif
