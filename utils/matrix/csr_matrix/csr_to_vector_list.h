#ifndef CSR_TO_VECTOR_LIST_H
#define CSR_TO_VECTOR_LIST_H

#include "csr_matrix.h"
#include "../vector_list/vector_list.h"

/**
 * @brief Choose random samples as cluster centers.
 *
 * @param[in] mtrx The input matrix to choose random samples from.
 * @param[out] clusters The output matrix consisting of random samples from mtrx.
 * @param[in] no_clusters length of clusters array.
 * @param[in] seed The seed used for the random number generator.
 * @param[out] chosen_elements The elements that were chosen from mtrx to serve as cluster centers.
 */
void create_vector_list_random(struct csr_matrix *mtrx
                                 , struct sparse_vector* clusters
                                 , uint64_t no_clusters
                                 , uint32_t *seed
                                 , uint64_t *chosen_elements);

/**
 * @brief Transform from matrix mtrx the vector vector_id to a block vector.
 *
 * @param[in] mtrx
 * @param[in] vector_id
 * @param[in] keys_per_block
 * @param[in] block_vector
 */
void create_block_vector_from_csr_matrix_vector(struct csr_matrix* mtrx
                                               , uint64_t vector_id
                                               , uint64_t keys_per_block
                                               , struct sparse_vector* block_vector);

/**
 * @brief Create the dot product of a sparse vector to a csr matrix.
 *
 * @param[in] keys of the sparse vector
 * @param[in] values values of the sparse vector
 * @param[in] nnz Length of keys/values
 * @param[in] mtrx The matrix to do the dot product with.
 * @param[out] result The resulting sparse vector of the dot product.
 */
void vector_matrix_dot(KEY_TYPE* keys,
                       VALUE_TYPE* values,
                       uint64_t nnz,
                       struct csr_matrix *mtrx,
                       struct sparse_vector *result);

/**
 * @brief From a input matrix mtrx1 with shape MxN and a second matrix
 *        mtrx2 with shape LxN create a matrix with shape MxL by doing the dot product.
 *
 * @param[in] mtrx1 The matrix to do the dot product with.
 * @param[in] mtrx2 The matrix to do the dot product with.
 * @return Resulting list of sparse vectors
 */
struct sparse_vector* matrix_dot(struct csr_matrix *mtrx1, struct csr_matrix *mtrx2);

/**
 * @brief From input sparse vectors with shape MxN and a second csr matrix
 *        mtrx with shape LxN create a sparse vectors with shape MxL by doing the dot product.
 *
 * @param[in] vectors The input sparse vectors.
 * @param[in] sample_count Length of the input sparse vectors.
 * @param[in] mtrx The matrix to do the dot product with.
 * @return Resulting list of sparse vectors
 */
struct sparse_vector* sparse_vectors_matrix_dot(struct sparse_vector* vectors,
                                                uint64_t sample_count,
                                                struct csr_matrix *mtrx);

/**
 * @brief From input sparse vectors with shape MxN and a second csr matrix
 *        mtrx with shape LxN create a sparse vectors with shape MxL by doing the dot product.
 *
 * @param[in] input_vectors The input sparse vectors.
 * @param[in] no_vectors Length of the input sparse vectors.
 * @param[in] mtrx The matrix to do the dot product with.
 * @param[in] vector_not_changed Vector with information which input_vector did change.
 * @param[out] result The resulting sparse vector of the dot product.
 */
void update_dot_products(struct sparse_vector *input_vectors,
                         uint64_t no_vectors,
                         struct csr_matrix *mtrx,
                         uint32_t* vector_not_changed,
                         struct sparse_vector *result);

/**
 * @brief Calculate the euclidean distance between one sample of mtrx1 and one sample
 *        of vector array mtrx2.
 *
 *
 * Calculate || mtrx1[sample_id_1] - mtrx2[sample_id_2] ||
 *
 * @param[in] mtrx1 First input matrix.
 * @param[in] sample_id_1 Id of sample from mtrx1 which is used for calculating the distance.
 * @param[in] mtrx2 Array of sparse vectors.
 * @param[in] sample_id_2 Id of sample from mtrx2 which is used for calculating the distance.
 * @param[in] vector_lengths_mtrx1 ||s||² for every s \in mtrx1.
 * @param[in] vector_lengths_mtrx2 ||s||² for every s \in mtrx2.
 * @return Result of euclidean distance calculation.
 */
VALUE_TYPE euclid_vector_list(struct csr_matrix *mtrx1, uint64_t sample_id_1
                         , struct sparse_vector *mtrx2, uint64_t sample_id_2
                         , VALUE_TYPE *vector_lengths_mtrx1
                         , VALUE_TYPE *vector_lengths_mtrx2);

#endif /* CSR_TO_VECTOR_LIST_H */
