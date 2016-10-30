#include "csr_to_vector_list.h"
#include "../../fcl_random.h"
#include "../../vector/common/common_vector_math.h"
#include "../../vector/sparse/sparse_vector_math.h"
#include <string.h>
#include <math.h>

void create_vector_list_random(struct csr_matrix *mtrx
                                 , struct sparse_vector* clusters
                                 , uint64_t no_clusters
                                 , uint32_t *seed) {
    uint64_t i, sample_id;

    /* calculate number of non zero values */
    for (i = 0; i < no_clusters; i++) {
        sample_id = rand_r(seed) % mtrx->sample_count;
        clusters[i].nnz = mtrx->pointers[sample_id + 1] - mtrx->pointers[sample_id];
        clusters[i].keys = (KEY_TYPE*) calloc(clusters[i].nnz, sizeof(KEY_TYPE));
        clusters[i].values = (VALUE_TYPE*) calloc(clusters[i].nnz, sizeof(VALUE_TYPE));
        memcpy(clusters[i].keys
               , mtrx->keys + mtrx->pointers[sample_id]
               , clusters[i].nnz * sizeof(KEY_TYPE));
        memcpy(clusters[i].values
               , mtrx->values + mtrx->pointers[sample_id]
               , clusters[i].nnz * sizeof(VALUE_TYPE));
    }
}

void create_block_vector_from_csr_matrix_vector(struct csr_matrix* mtrx
                                               , uint64_t vector_id
                                               , uint64_t keys_per_block
                                               , struct sparse_vector* block_vector) {

    block_vector->keys = (KEY_TYPE*) calloc(mtrx->pointers[vector_id + 1] - mtrx->pointers[vector_id], sizeof(KEY_TYPE));
    block_vector->values = (VALUE_TYPE*) calloc(mtrx->pointers[vector_id + 1] - mtrx->pointers[vector_id], sizeof(VALUE_TYPE));

    fill_blockvector(mtrx->keys + mtrx->pointers[vector_id]
                          , mtrx->values + mtrx->pointers[vector_id]
                          , mtrx->pointers[vector_id + 1] - mtrx->pointers[vector_id]
                          , keys_per_block
                          , block_vector->keys
                          , block_vector->values
                          , &(block_vector->nnz));
}

VALUE_TYPE euclid_vector_list(struct csr_matrix *mtrx1, uint64_t sample_id_1
                         , struct sparse_vector *mtrx2, uint64_t sample_id_2
                         , VALUE_TYPE *vector_lengths_mtrx1
                         , VALUE_TYPE *vector_lengths_mtrx2) {

     return sqrt(
                value_type_max(
                    vector_lengths_mtrx1[sample_id_1]
                    + vector_lengths_mtrx2[sample_id_2]
                    - 2 * dot(mtrx1->keys + mtrx1->pointers[sample_id_1]
                           , mtrx1->values + mtrx1->pointers[sample_id_1]
                           , mtrx1->pointers[sample_id_1 + 1]
                                                 - mtrx1->pointers[sample_id_1]
                           , mtrx2[sample_id_2].keys
                           , mtrx2[sample_id_2].values
                           , mtrx2[sample_id_2].nnz)
                    , 0.0)
            );

}
