#include "csr_math.h"
#include "../../vector/common/common_vector_math.h"
#include "../../vector/sparse/sparse_vector_math.h"
#include "../../fcl_logging.h"

#include <stdlib.h>
#include <math.h>

void calculate_matrix_vector_lengths(struct csr_matrix *mtrx, VALUE_TYPE** vector_lengths) {
    uint64_t i;

    *vector_lengths = (VALUE_TYPE*) calloc(mtrx->sample_count, sizeof(VALUE_TYPE));

    #pragma omp parallel for schedule(dynamic, 1000)
    for (i = 0; i < mtrx->sample_count; i++) {
        (*vector_lengths)[i] = calculate_squared_vector_length(mtrx->values + mtrx->pointers[i]
                                                                     , mtrx->pointers[i + 1] - mtrx->pointers[i]);
    }

}

void create_block_vectors_from_matrix(struct csr_matrix *mtrx, uint64_t no_blocks, struct csr_matrix *block_vectors) {
    uint64_t i,  keys_per_block;

    block_vectors->sample_count = mtrx->sample_count;
    block_vectors->dim = no_blocks;
    block_vectors->pointers = (POINTER_TYPE*) calloc(mtrx->sample_count + 1, sizeof(POINTER_TYPE));
    keys_per_block = mtrx->dim / no_blocks;

    if (mtrx->dim % no_blocks > 0) keys_per_block++;

    for (i = 0; i < mtrx->sample_count; i++) {
        block_vectors->pointers[i + 1] = get_blockvector_nnz(mtrx->keys + mtrx->pointers[i]
                                                            , mtrx->values + mtrx->pointers[i]
                                                            , mtrx->pointers[i + 1] - mtrx->pointers[i]
                                                            , keys_per_block);

        block_vectors->pointers[i + 1] += block_vectors->pointers[i];
    }

    block_vectors->keys = (KEY_TYPE*) calloc(block_vectors->pointers[mtrx->sample_count], sizeof(KEY_TYPE));
    block_vectors->values = (VALUE_TYPE*) calloc(block_vectors->pointers[mtrx->sample_count], sizeof(VALUE_TYPE));

    #pragma omp parallel for schedule(dynamic, 1000)
    for (i = 0; i < mtrx->sample_count; i++) {
        uint64_t nnz_bv;
        nnz_bv = 0;

        fill_blockvector(mtrx->keys + mtrx->pointers[i]
                              , mtrx->values + mtrx->pointers[i]
                              , mtrx->pointers[i + 1] - mtrx->pointers[i]
                              , keys_per_block
                              , block_vectors->keys + block_vectors->pointers[i]
                              , block_vectors->values + block_vectors->pointers[i]
                              , &nnz_bv);

        if (nnz_bv != block_vectors->pointers[i + 1] - block_vectors->pointers[i]) {
            LOG_ERROR("ERROR: generated block vectors matrix has incorrect items!");
        }
    }
}

/*
 * Calc the euclidean distance between two sparse vectors
 * Assumption dimensions of dense matrix == dim_matrix
 *
 */

VALUE_TYPE euclid(struct csr_matrix *mtrx1, uint64_t sample_id_1
                         , struct csr_matrix *mtrx2, uint64_t sample_id_2
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
                           , mtrx2->keys + mtrx2->pointers[sample_id_2]
                           , mtrx2->values + mtrx2->pointers[sample_id_2]
                           , mtrx2->pointers[sample_id_2 + 1]
                                                 - mtrx2->pointers[sample_id_2])
                    , 0.0)
            );

}

uint64_t get_average_nnz_from_blockvector_matrix(struct csr_matrix* mtrx
                                                , uint64_t no_blocks) {
    uint64_t nnz;
    uint64_t i;
    uint64_t keys_per_block;

    keys_per_block = mtrx->dim / no_blocks;
    if (mtrx->dim % no_blocks > 0) keys_per_block++;

    nnz = 0;

    #pragma omp parallel for schedule(dynamic, 1000) reduction(+:nnz)
    for (i = 0; i < mtrx->sample_count; i++) {
        nnz += get_blockvector_nnz(mtrx->keys + mtrx->pointers[i]
                                  , mtrx->values + mtrx->pointers[i]
                                  , mtrx->pointers[i + 1] - mtrx->pointers[i]
                                  , keys_per_block);
    }

    return (uint64_t) (nnz / mtrx->sample_count);
}

uint64_t search_block_vector_size(struct csr_matrix* samples
                                  , VALUE_TYPE desired_annz, uint32_t verbose) {

    uint64_t annz_block_vectors, annz_samples;
    uint64_t block_vectors_dim;

    block_vectors_dim = samples->dim * desired_annz * 1.33334;
    if (block_vectors_dim > samples->dim) block_vectors_dim = samples->dim;
    if (block_vectors_dim == 0) block_vectors_dim = 1;

    annz_block_vectors = get_average_nnz_from_blockvector_matrix(samples, block_vectors_dim);
    annz_samples = samples->pointers[samples->sample_count] / samples->sample_count;

    while((annz_block_vectors > (desired_annz * annz_samples)) && (block_vectors_dim > 1)) {
        block_vectors_dim = block_vectors_dim * 0.7;
        if (block_vectors_dim == 0) block_vectors_dim = 1;
        annz_block_vectors = get_average_nnz_from_blockvector_matrix(samples, block_vectors_dim);
    }

    if (verbose) LOG_INFO("n_blockvector_samples = %" PRINTF_INT64_MODIFIER "u, average_nnz_blockvektor_samples = %" PRINTF_INT64_MODIFIER "u"
            , block_vectors_dim
            , annz_block_vectors);
    fflush(stdout);

    return block_vectors_dim;
}

void determine_block_vectors_for_matrix(struct csr_matrix* mtrx
                                   , VALUE_TYPE desired_annz
                                   , struct csr_matrix* block_vectors_mtrx
                                   , uint64_t *block_vectors_dim) {

    uint64_t annz_block_vectors, annz_mtrx;

    *block_vectors_dim = mtrx->dim * desired_annz * 1.33334;
    if (*block_vectors_dim > mtrx->dim) *block_vectors_dim = mtrx->dim;
    if (*block_vectors_dim == 0) *block_vectors_dim = 1;

    create_block_vectors_from_matrix(mtrx
                                     , *block_vectors_dim
                                     , block_vectors_mtrx);

    annz_block_vectors = block_vectors_mtrx->pointers[block_vectors_mtrx->sample_count] / block_vectors_mtrx->sample_count;
    annz_mtrx = mtrx->pointers[mtrx->sample_count] / mtrx->sample_count;

    while((annz_block_vectors > (desired_annz * annz_mtrx))
          && (*block_vectors_dim > 1)) {

        free_csr_matrix(block_vectors_mtrx);
        *block_vectors_dim = *block_vectors_dim * 0.7;
        if (*block_vectors_dim == 0) *block_vectors_dim = 1;

        create_block_vectors_from_matrix(mtrx
                                         , *block_vectors_dim
                                         , block_vectors_mtrx);

        annz_block_vectors = block_vectors_mtrx->pointers[block_vectors_mtrx->sample_count] / block_vectors_mtrx->sample_count;
    }
}

