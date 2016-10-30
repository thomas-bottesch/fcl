#ifndef CSR_MATH_H
#define CSR_MATH_H


#include "csr_matrix.h"

/**
 * @brief Calculate ||s||² for all s \in mtrx.
 *
 * @param[in] mtrx is the input matrix
 * @param[out] vector_lengths array of ||s||² for all s \in mtrx
 */
void calculate_matrix_vector_lengths(struct csr_matrix *mtrx
                                            , VALUE_TYPE** vector_lengths);

/**
 * @brief When converting a matrix to a block vector matrix a specific condition needs to
 *        be met defined within this function. The resulting block vector size returned here
 *        is one value which meets this condition.
 *
 * @param[in] samples which shall be clustered.
 * @param[in] desired_annz Desired size of the resulting block vector 0 to 100 percent
 * @param[in] verbose Flag to activate verbosity.
 * @return The number of blocks the blockvectors need to have to meet the condition.
 */
uint64_t search_block_vector_size(struct csr_matrix* samples
                                  , VALUE_TYPE desired_annz, uint32_t verbose);

/**
 * @brief Iteratively searches for a block vector matrix until average nnz of the block
 *        vector matrix is desired_annz * average nnz of the mtrx.
 *
 * @param[in] mtrx which shall be clustered.
 * @param[in] desired_annz Desired size of the resulting block vector 0 to 100 percent
 * @param[out] block_vectors_mtrx is the matrix of resulting block vectors
 * @param[out] block_vectors_dim is the dimensionality of the block vectors
 */
void determine_block_vectors_for_matrix(struct csr_matrix* mtrx
                                   , VALUE_TYPE desired_annz
                                   , struct csr_matrix* block_vectors_mtrx
                                   , uint64_t *block_vectors_dim);

/**
 * @brief Create block vector matrix from a given matrix.
 *
 * @param[in] mtrx Matrix the block vectors should be created from.
 * @param[in] no_block_vectors The number of blocks that should be used per vector.
 * @param[out] block_vectors The resulting block vector matrix.
 */
void create_block_vectors_from_matrix(struct csr_matrix *mtrx, uint64_t no_block_vectors, struct csr_matrix *block_vectors);

/**
 * @brief Calculate the euclidean distance between one sample of mtrx1 and one sample
 *        of mtrx2.
 *
 *
 * Calculate || mtrx1[sample_id_1] - mtrx2[sample_id_2] ||
 *
 * @param[in] mtrx1 First input matrix.
 * @param[in] sample_id_1 Id of sample from mtrx1 which is used for calculating the distance.
 * @param[in] mtrx2 Second input matrix.
 * @param[in] sample_id_2 Id of sample from mtrx2 which is used for calculating the distance.
 * @param[in] vector_lengths_mtrx1 ||s||² for every s \in mtrx1.
 * @param[in] vector_lengths_mtrx2 ||s||² for every s \in mtrx2.
 * @return Result of euclidean distance calculation.
 */
VALUE_TYPE euclid(struct csr_matrix *mtrx1, uint64_t sample_id_1
                         , struct csr_matrix *mtrx2, uint64_t sample_id_2
                         , VALUE_TYPE *vector_lengths_mtrx1
                         , VALUE_TYPE *vector_lengths_mtrx2);

#endif /* CSR_MATH_H */
