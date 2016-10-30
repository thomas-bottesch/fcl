#include "vector_list_math.h"
#include "../../vector/common/common_vector_math.h"
#include "../../vector/sparse/sparse_vector_math.h"
#include "../../global_defs.h"
#include <stdlib.h>

void update_vector_list_lengths(struct sparse_vector* vector_array
                                   , uint64_t no_clusters
                                   , uint32_t* cluster_not_changed
                                   , VALUE_TYPE* vector_lengths) {
    uint64_t i;

    #pragma omp parallel for schedule(dynamic, 1000)
    for (i = 0; i < no_clusters; i++) {
        if (!cluster_not_changed[i]) {
            vector_lengths[i] = calculate_squared_vector_length(vector_array[i].values
                                                                     , vector_array[i].nnz);
        }
    }
}

void calculate_vector_list_lengths(struct sparse_vector* vector_array
                                   , uint64_t no_clusters
                                   , VALUE_TYPE** vector_lengths) {

    uint64_t i;

    *vector_lengths = (VALUE_TYPE*) calloc(no_clusters, sizeof(VALUE_TYPE));

    #pragma omp parallel for schedule(dynamic, 1000)
    for (i = 0; i < no_clusters; i++) {
        (*vector_lengths)[i] = calculate_squared_vector_length(vector_array[i].values
                                                                     , vector_array[i].nnz);
    }
}

void create_block_vectors_list_from_vector_list(struct sparse_vector *mtrx
                                                , uint64_t no_blocks
                                                , uint64_t no_vectors
                                                , uint64_t dim
                                                , struct sparse_vector **block_vectors) {
    uint64_t i,  keys_per_block;
    keys_per_block = dim / no_blocks;
    if (dim % no_blocks > 0) keys_per_block++;

    *block_vectors = (struct sparse_vector *) calloc(no_vectors, sizeof(struct sparse_vector));

    #pragma omp parallel for schedule(dynamic, 1000)
    for (i = 0; i < no_vectors; i++) {
        (*block_vectors)[i].nnz = get_blockvector_nnz(mtrx[i].keys
                                                  , mtrx[i].values
                                                  , mtrx[i].nnz
                                                  , keys_per_block);

        if ((*block_vectors)[i].nnz > 0) {
            (*block_vectors)[i].keys = (KEY_TYPE*) calloc((*block_vectors)[i].nnz, sizeof(KEY_TYPE));
            (*block_vectors)[i].values = (VALUE_TYPE*) calloc((*block_vectors)[i].nnz, sizeof(VALUE_TYPE));

            fill_blockvector(mtrx[i].keys
                                  , mtrx[i].values
                                  , mtrx[i].nnz
                                  , keys_per_block
                                  , (*block_vectors)[i].keys
                                  , (*block_vectors)[i].values
                                  , &((*block_vectors)[i].nnz));
        }
    }
}

void update_changed_blockvectors(struct sparse_vector *mtrx
                                , uint64_t no_blocks
                                , uint64_t no_vectors
                                , uint64_t dim
                                , uint32_t* vector_not_changed
                                , struct sparse_vector *block_vectors) {
    uint64_t i,  keys_per_block;
    keys_per_block = dim / no_blocks;
    if (dim % no_blocks > 0) keys_per_block++;

    #pragma omp parallel for schedule(dynamic, 1000)
    for (i = 0; i < no_vectors; i++) {
        if (vector_not_changed[i]) continue;

        free_null(block_vectors[i].keys);
        free_null(block_vectors[i].values);

        block_vectors[i].nnz = get_blockvector_nnz(mtrx[i].keys
                                                  , mtrx[i].values
                                                  , mtrx[i].nnz
                                                  , keys_per_block);

        if (block_vectors[i].nnz > 0) {
            block_vectors[i].keys = (KEY_TYPE*) calloc(block_vectors[i].nnz, sizeof(KEY_TYPE));
            block_vectors[i].values = (VALUE_TYPE*) calloc(block_vectors[i].nnz, sizeof(VALUE_TYPE));

            fill_blockvector(mtrx[i].keys
                                  , mtrx[i].values
                                  , mtrx[i].nnz
                                  , keys_per_block
                                  , block_vectors[i].keys
                                  , block_vectors[i].values
                                  , &(block_vectors[i].nnz));
        }
    }
}

