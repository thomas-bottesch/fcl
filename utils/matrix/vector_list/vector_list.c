#include "vector_list.h"
#include "../../global_defs.h"
#include <stdlib.h>

void free_vector_list(struct sparse_vector* vec_list, uint64_t no_vectors) {
    uint64_t i;

    for (i = 0; i  < no_vectors; i++) {
        free_null(vec_list[i].keys);
        free_null(vec_list[i].values);
    }
}
