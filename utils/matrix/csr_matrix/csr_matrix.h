#ifndef CSR_MATRIX_H
#define CSR_MATRIX_H

#include "../../types.h"
#include "../../global_defs.h"

/**
 * @brief Compressed Sparse Row matrix
 *
 * Look at https://en.wikipedia.org/wiki/Sparse_matrix for more details.
 */
struct csr_matrix {
    KEY_TYPE *keys;                /**< Keys of the csr matrix. */
    VALUE_TYPE *values;            /**< Values of the csr matrix. */
    POINTER_TYPE *pointers;        /**< Pointer indices of the csr matrix. */
    uint64_t sample_count;         /**< Number of samples in the matrix */
    uint64_t dim;                  /**< Number of features of the matrix */
};

/**
 * @brief Cleanup csr matrix.
 *
 * @param mtrx[in] Matrix to cleanup.
 */
void free_csr_matrix(struct csr_matrix *mtrx);

/**
 * @brief Initialize all fields of a csr matrix with zero.
 *
 * @param mtrx[in] Matrix to initialize.
 */
void initialize_csr_matrix_zero(struct csr_matrix *mtrx);

/**
 * @brief Remove all vectors from a matrix which are not set in a mask.
 *
 * @param[in] clusters The matrix to remove samples from.
 * @param[in] mask Array of true false. True means keep vector, false remove vector.
 * @return New matrix without the vectors that had a mask value of false.
 */
struct csr_matrix* remove_vectors_not_in_mask(struct csr_matrix* clusters
                                                   , uint64_t* mask);

/**
 * @brief Choose random samples from mtrx to generate mtrx2.
 *
 * NOTE: mtrx2->sample_count is already set and determines how many samples are
 * chosen.
 *
 * @param[in] mtrx The input matrix to choose random samples from.
 * @param[out] mtrx2 The output matrix consisting of random samples of mtrx.
 * @param[in] seed The seed used for the random number generator.
 */
void create_matrix_random(struct csr_matrix *mtrx
                                 , struct csr_matrix *mtrx2
                                 , uint32_t *seed);

#endif /* CSR_MATRIX_H */
