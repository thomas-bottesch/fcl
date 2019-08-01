#include "minibatch_commons.h"
#include "../../utils/global_defs.h"

void create_chosen_sample_map(uint32_t** chosen_sample_map
                             , uint64_t no_samples
                             , uint64_t batch_size
                             , unsigned int* seed) {
    uint64_t j;

    free_null(*chosen_sample_map);
    *chosen_sample_map = (uint32_t*) calloc(no_samples, sizeof(uint32_t));
    for (j = 0; j < batch_size; j++) {
        (*chosen_sample_map)[rand_r(seed) % no_samples] = 1;
    }
}
