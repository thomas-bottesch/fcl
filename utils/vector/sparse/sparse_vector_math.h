#ifndef SPARSE_VECTOR_MATH_H
#define SPARSE_VECTOR_MATH_H

#include "../../types.h"
#include "../common/common_vector_math.h"

/**
 *
 * @param[in] keys of the sparse vector
 * @param[in] values values of the sparse vector
 * @param[in] nnz Length of keys/values
 * @param[in] keys_per_block Is the number of keys to combine to one block.
 * @param blockvector_keys of the block vector
 * @param blockvector_values of the block vector
 * @param bv_nnz Length of blockvector_keys/blockvector_values
 */
void fill_blockvector(KEY_TYPE* keys
                      , VALUE_TYPE* values
                      , uint64_t nnz
                      , uint64_t keys_per_block
                      , KEY_TYPE* blockvector_keys
                      , VALUE_TYPE* blockvector_values
                      , uint64_t* bv_nnz);

/**
 * @brief For a given sparse vector with keys, values and nnz calculate how many nnz
 *        the vector would have as a blockvector when combining keys_per_block features
 *        into one feature.
 *
 * @param[in] keys of the sparse vector
 * @param[in] values values of the sparse vector
 * @param[in] nnz Length of keys/values
 * @param[in] keys_per_block Is the number of keys to combine to one block.
 * @return Resulting nnz of sparse vector if converting to block vector.
 */
uint64_t get_blockvector_nnz(KEY_TYPE* keys
                             , VALUE_TYPE* values
                             , uint64_t nnz
                             , uint64_t keys_per_block);

/**
 * @brief Calculate dot product between two sparse vectors.
 *
 * @param[in] keys_vector_one Array of keys first vector.
 * @param[in] values_vector_one Array of keys first vector.
 * @param[in] non_zero_count_vector_one Number of non zero values first vector.
 * @param[in] keys_vector_two Array of keys second vector.
 * @param[in] values_vector_two Array of keys second vector.
 * @param[in] non_zero_count_vector_two Number of non zero values second vector.
 * @return Result of the dot product.
 */
VALUE_TYPE dot(KEY_TYPE *keys_vector_one
                                , VALUE_TYPE *values_vector_one
                                , uint64_t non_zero_count_vector_one
                                , KEY_TYPE *keys_vector_two
                                , VALUE_TYPE *values_vector_two
                                , uint64_t non_zero_count_vector_two);

/**
 * @brief Calculate euclidean distance between two sparse samples.
 *
 * @param[in] keys_vector_one Array of keys first vector.
 * @param[in] values_vector_one Array of keys first vector.
 * @param[in] non_zero_count_vector_one Number of non zero values first vector.
 * @param[in] keys_vector_two Array of keys second vector.
 * @param[in] values_vector_two Array of keys second vector.
 * @param[in] non_zero_count_vector_two Number of non zero values second vector.
 * @param[in] vector_one_length_squared Length of first vector squared.
 * @param[in] vector_two_length_squared Length of second vector squared.
 * @return Result of euclidean distance calculation.
 */
VALUE_TYPE euclid_vector(KEY_TYPE *keys_vector_one
                        , VALUE_TYPE *values_vector_one
                        , uint64_t non_zero_count_vector_one
                        , KEY_TYPE *keys_vector_two
                        , VALUE_TYPE *values_vector_two
                        , uint64_t non_zero_count_vector_two
                        , VALUE_TYPE vector_one_length_squared
                        , VALUE_TYPE vector_two_length_squared);


#endif /* SPARSE_VECTOR_MATH_H */
