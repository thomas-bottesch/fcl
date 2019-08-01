#include "kmeans.h"
#include "kmeans_utils.h"

#ifndef MINIBATCH_COMMONS_H
#define MINIBATCH_COMMONS_H

void create_chosen_sample_map(uint32_t** chosen_sample_map
                             , uint64_t no_samples
                             , uint64_t batch_size
                             , unsigned int* seed);
                                       
#endif