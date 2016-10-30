#include "vector_list_to_csr.h"
#include <string.h>

void sparse_vector_list_to_csr_matrix(struct sparse_vector* mtrx
                                      , uint64_t no_vectors
                                      , uint64_t dim
                                      , struct csr_matrix* csr_mtrx) {
    uint64_t i;

    csr_mtrx->sample_count = no_vectors;
    csr_mtrx->dim = dim;
    csr_mtrx->pointers = (POINTER_TYPE*) calloc(csr_mtrx->sample_count + 1, sizeof(POINTER_TYPE));

    for (i = 0; i < no_vectors; i++) {
        csr_mtrx->pointers[i + 1] = mtrx[i].nnz + csr_mtrx->pointers[i];
    }

    csr_mtrx->keys = (KEY_TYPE*) calloc(csr_mtrx->pointers[csr_mtrx->sample_count], sizeof(KEY_TYPE));
    csr_mtrx->values = (VALUE_TYPE*) calloc(csr_mtrx->pointers[csr_mtrx->sample_count], sizeof(VALUE_TYPE));

    for (i = 0; i < no_vectors; i++) {
        memcpy(csr_mtrx->keys + csr_mtrx->pointers[i], mtrx[i].keys, mtrx[i].nnz * sizeof(KEY_TYPE));
        memcpy(csr_mtrx->values + csr_mtrx->pointers[i], mtrx[i].values, mtrx[i].nnz * sizeof(VALUE_TYPE));
    }
}

struct csr_matrix* create_matrix_without_empty_elements(struct sparse_vector* mtrx
                                                           , uint64_t no_vectors
                                                           , uint64_t dim
                                                           , uint64_t* mask) {
    uint64_t i, nnz;
    uint64_t no_filled;
    struct csr_matrix* res;

    nnz = 0;
    no_filled = 0;
    for (i = 0; i < no_vectors; i++) {
        if (mask == NULL || mask[i] != 0) {
            no_filled += 1;
            nnz += mtrx[i].nnz;
        }
    }

    res = (struct csr_matrix*) calloc(1, sizeof(struct csr_matrix));
    res->sample_count = no_filled;
    res->dim = dim;
    res->pointers = (POINTER_TYPE*) calloc(res->sample_count + 1, sizeof(POINTER_TYPE));

    no_filled = 0;
    for (i = 0; i < no_vectors; i++) {
        if (mask == NULL || mask[i] != 0) {
            res->pointers[no_filled + 1] = mtrx[i].nnz + res->pointers[no_filled];
            no_filled += 1;
        }
    }

    res->keys = (KEY_TYPE*) calloc(res->pointers[res->sample_count], sizeof(KEY_TYPE));
    res->values = (VALUE_TYPE*) calloc(res->pointers[res->sample_count], sizeof(VALUE_TYPE));

    no_filled = 0;
    for (i = 0; i < no_vectors; i++) {
        if (mask == NULL || mask[i] != 0) {
            memcpy(res->keys + res->pointers[no_filled], mtrx[i].keys, mtrx[i].nnz * sizeof(KEY_TYPE));
            memcpy(res->values + res->pointers[no_filled], mtrx[i].values, mtrx[i].nnz * sizeof(VALUE_TYPE));
            no_filled += 1;
        }
    }

    return res;
}
