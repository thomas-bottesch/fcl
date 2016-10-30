#include "csr_matrix.h"
#include "stdlib.h"
#include "string.h"

void initialize_csr_matrix_zero(struct csr_matrix *mtrx) {
    mtrx->pointers = NULL;
    mtrx->keys = NULL;
    mtrx->values = NULL;
    mtrx->sample_count = 0;
    mtrx->dim = 0;
}

void free_csr_matrix(struct csr_matrix *mtrx) {
    free_null(mtrx->pointers);
    free_null(mtrx->keys);
    free_null(mtrx->values);
}

struct csr_matrix* remove_vectors_not_in_mask(struct csr_matrix* clusters
                                                   , uint64_t* mask) {
    uint64_t i, no_vectors_in_mask, nnz;
    struct csr_matrix* cluster_without_empty;

    no_vectors_in_mask = 0;

    for (i = 0; i < clusters->sample_count; i++) {
        if (mask[i] != 0) {
            no_vectors_in_mask += 1;
        }
    }
    cluster_without_empty = (struct csr_matrix*) malloc(sizeof(struct csr_matrix));
    cluster_without_empty->dim = clusters->dim;
    cluster_without_empty->sample_count = no_vectors_in_mask;
    cluster_without_empty->pointers = (POINTER_TYPE*) calloc(no_vectors_in_mask + 1
                                                             , sizeof(POINTER_TYPE));
    cluster_without_empty->keys = (KEY_TYPE*) calloc(clusters->pointers[clusters->sample_count], sizeof(KEY_TYPE));
    cluster_without_empty->values = (VALUE_TYPE*) calloc(clusters->pointers[clusters->sample_count], sizeof(VALUE_TYPE));

    no_vectors_in_mask = 0;
    for (i = 0; i < clusters->sample_count; i++) {
        if (mask[i] != 0) {
            nnz = clusters->pointers[i + 1] - clusters->pointers[i];
            memcpy(cluster_without_empty->keys + cluster_without_empty->pointers[no_vectors_in_mask], clusters->keys + clusters->pointers[i], nnz * sizeof(KEY_TYPE));
            memcpy(cluster_without_empty->values + cluster_without_empty->pointers[no_vectors_in_mask], clusters->values + clusters->pointers[i], nnz * sizeof(VALUE_TYPE));
            no_vectors_in_mask += 1;

            cluster_without_empty->pointers[no_vectors_in_mask]
                     = cluster_without_empty->pointers[no_vectors_in_mask - 1] + nnz;
        }
    }
    return cluster_without_empty;
}

void create_matrix_random(struct csr_matrix *mtrx
                                 , struct csr_matrix *mtrx2
                                 , uint32_t *seed) {
    uint64_t i, nnz, sample_id;
    uint32_t seed_save;
    nnz = 0;

    seed_save = *seed;

    /* calculate number of non zero values */
    for (i = 0; i < mtrx2->sample_count; i++) {
        sample_id = rand_r(seed) % mtrx->sample_count;
        nnz += mtrx->pointers[sample_id + 1] - mtrx->pointers[sample_id];
        mtrx2->pointers[i + 1] = nnz;
    }

    /* allocate sparse cluster matrix */
    mtrx2->keys = (KEY_TYPE*) calloc(nnz, sizeof(KEY_TYPE));
    mtrx2->values = (VALUE_TYPE*) calloc(nnz, sizeof(VALUE_TYPE));

    *seed = seed_save;

    /* fill sparse cluster matrix */
    for (i = 0; i < mtrx2->sample_count; i++) {
        sample_id = rand_r(seed) % mtrx->sample_count;
        nnz = mtrx2->pointers[i + 1] - mtrx2->pointers[i];
        if (nnz > 0) {
            memcpy(mtrx2->keys + mtrx2->pointers[i], mtrx->keys + mtrx->pointers[sample_id], nnz * sizeof(KEY_TYPE));
            memcpy(mtrx2->values + mtrx2->pointers[i], mtrx->values + mtrx->pointers[sample_id], nnz * sizeof(VALUE_TYPE));
        }
    }
}
