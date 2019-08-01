#include "csr_to_vector_list.h"
#include "../../fcl_random.h"
#include "../../vector/common/common_vector_math.h"
#include "../../vector/sparse/sparse_vector_math.h"
#include <string.h>
#include <math.h>

void create_vector_list_random(struct csr_matrix *mtrx
                                 , struct sparse_vector* clusters
                                 , uint64_t no_clusters
                                 , uint32_t *seed
                                 , uint64_t *chosen_elements) {
    uint64_t i, sample_id;

    /* calculate number of non zero values */
    for (i = 0; i < no_clusters; i++) {
        sample_id = rand_r(seed) % mtrx->sample_count;
        if (chosen_elements) chosen_elements[i] = sample_id;
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

void update_dot_products(struct sparse_vector *input_vectors,
                         uint64_t no_vectors,
                         struct csr_matrix *mtrx,
                         uint32_t* vector_not_changed,
                         struct sparse_vector *result) {
    /* some of the input_vectors changed and the result of the dot product
     * of input_vectors with mtrx needs to be recalculated.
     */
    uint64_t i;

    #pragma omp parallel for schedule(dynamic, 1000)
    for (i = 0; i < no_vectors; i++) {
        if (vector_not_changed[i]) continue;

        free_null(result[i].keys);
        free_null(result[i].values);

        vector_matrix_dot(input_vectors[i].keys,
                          input_vectors[i].values,
                          input_vectors[i].nnz,
                          mtrx,
                          result + i);
    }
}

void vector_matrix_dot(KEY_TYPE* keys,
                       VALUE_TYPE* values,
                       uint64_t nnz,
                       struct csr_matrix *mtrx,
                       struct sparse_vector *result) {

    VALUE_TYPE *value_result_padded = (VALUE_TYPE*) calloc(mtrx->sample_count, sizeof(VALUE_TYPE));
    KEY_TYPE *key_result_padded = (KEY_TYPE*) calloc(mtrx->sample_count, sizeof(KEY_TYPE));
    uint64_t nnz_res, j;

    nnz_res = 0;
    for (j = 0; j < mtrx->sample_count; j++) {
        VALUE_TYPE dot_result;
        dot_result = dot(keys,
                         values,
                         nnz,
                         mtrx->keys + mtrx->pointers[j],
                         mtrx->values + mtrx->pointers[j],
                         mtrx->pointers[j + 1] - mtrx->pointers[j]);

        if (dot_result != 0) {
            value_result_padded[nnz_res] = dot_result;
            key_result_padded[nnz_res] = j;
            nnz_res += 1;
        }
    }

    result->nnz = nnz_res;
    result->keys = (KEY_TYPE*) calloc(nnz_res, sizeof(KEY_TYPE));
    result->values = (VALUE_TYPE*) calloc(nnz_res, sizeof(VALUE_TYPE));
    memcpy(result->keys,
           key_result_padded,
           result->nnz * sizeof(KEY_TYPE));
    memcpy(result->values,
           value_result_padded,
           result->nnz * sizeof(VALUE_TYPE));
    free(value_result_padded);
    free(key_result_padded);
}

struct sparse_vector* matrix_dot(struct csr_matrix *mtrx1, struct csr_matrix *mtrx2) {
    /* From a input matrix mtrx with shape MxN and a second matrix
     * mtrx2 with shape LxN create a matrix with shape MxL.
     */
    struct sparse_vector *result =
        (struct sparse_vector *) calloc(mtrx1->sample_count, sizeof(struct sparse_vector));

    uint64_t i;
    for (i = 0; i < mtrx1->sample_count; i++) {
        vector_matrix_dot(mtrx1->keys + mtrx1->pointers[i],
                          mtrx1->values + mtrx1->pointers[i],
                          mtrx1->pointers[i + 1] - mtrx1->pointers[i],
                          mtrx2,
                          result + i);
    }

    return result;
}

struct sparse_vector* sparse_vectors_matrix_dot(struct sparse_vector* vectors,
                                                uint64_t sample_count,
                                                struct csr_matrix *mtrx) {
    /* From a input matrix mtrx with shape MxN and a second matrix
     * mtrx2 with shape LxN create a matrix with shape MxL.
     */
    struct sparse_vector *result =
        (struct sparse_vector *) calloc(sample_count, sizeof(struct sparse_vector));

    uint64_t i;
    for (i = 0; i < sample_count; i++) {
        vector_matrix_dot(vectors[i].keys,
                          vectors[i].values,
                          vectors[i].nnz,
                          mtrx,
                          result + i);
    }

    return result;
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
