#ifndef VECTOR_LIST_MATH_H
#define VECTOR_LIST_MATH_H

#include "vector_list.h"

/**
 * @brief Calculate ||s||² for all s \in vector_array.
 *
 * @param[in] vector_array Array of sparse vectors.
 * @param[in] no_clusters Length of vector array
 * @param[out] vector_lengths array of ||s||² for all s \in vector_array
 */
void calculate_vector_list_lengths(struct sparse_vector* vector_array
                                   , uint64_t no_clusters
                                   , VALUE_TYPE** vector_lengths);

/**
 * Recalculates only the vector length for vectors that have changed!
 *
 * @param[in] vector_array vectors for which to calculate ||v||
 * @param[in] no_clusters length of vector array
 * @param[in] cluster_not_changed List of true/false.
 *                            If cluster_not_changed[i] == false
                              then vector_lengths[i] needs to be updated!
 * @param[inout] vector_lengths vector length which shall be updated
 */
void update_vector_list_lengths(struct sparse_vector* vector_array
                                   , uint64_t no_clusters
                                   , uint32_t* cluster_not_changed
                                   , VALUE_TYPE* vector_lengths);

/**
 * Update a list of sparse block vectors from a list of sparse vectors.
 * Only the block vectors are updated where the vectors in mtrx were changed.
 *
 * @param[in] mtrx list of sparse vectors
 * @param[in] no_blocks equals the dimensionality of the resulting block vectors
 * @param[in] no_vectors length of mtrx and length of block_vectors
 * @param[in] dim dimensionality of mtrx
 * @param[in] List of True/False for every vector in mtrx. True if vector did not change.
 * @param[inout] updated block_vectors.
 */
void update_changed_blockvectors(struct sparse_vector *mtrx
                                , uint64_t no_blocks
                                , uint64_t no_vectors
                                , uint64_t dim
                                , uint32_t* vector_not_changed
                                , struct sparse_vector *block_vectors);

/**
 * Create a list of sparse block vectors from a list of sparse vectors
 * @param[in] mtrx list of sparse vectors
 * @param[in] no_blocks equals the dimensionality of the resulting block vectors
 * @param[in] no_vectors length of mtrx and length of block_vectors
 * @param[in] dim dimensionality of mtrx
 * @param[out] block_vectors Resulting block vectors.
 */
void create_block_vectors_list_from_vector_list(struct sparse_vector *mtrx
                                                , uint64_t no_blocks
                                                , uint64_t no_vectors
                                                , uint64_t dim
                                                , struct sparse_vector **block_vectors);

#endif /* VECTOR_LIST_MATH_H */
