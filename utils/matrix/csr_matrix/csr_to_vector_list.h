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
 */
void create_vector_list_random(struct csr_matrix *mtrx
                                 , struct sparse_vector* clusters
                                 , uint64_t no_clusters
                                 , unsigned int *seed);

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
