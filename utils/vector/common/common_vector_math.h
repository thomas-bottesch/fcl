#ifndef COMMON_VECTOR_MATH_H
#define COMMON_VECTOR_MATH_H

#include "../../types.h"

/**
 * @brief Determine how many elements are not zero within an array.
 *
 * @param[in] array Array which shall be analysed.
 * @param[in] no_elements Length of array.
 * @return The number of elements which are not zero.
 */
uint64_t get_nnz_uint64_array(uint64_t* array, uint64_t no_elements);

/**
 * @brief Calculate ||s||² for a vector.
 *
 * @param[in] vector The vector to created ||s||² for.
 * @param[in] dim The length of the vector.
 * @return The squared length of the vector.
 */
VALUE_TYPE calculate_squared_vector_length(VALUE_TYPE *vector, uint64_t dim);

/**
 * @brief Calculate the sum of all elements of an array.
 *
 * @param[in] array Array which should be summed up.
 * @param[in] no_elements Length of array.
 * @return Sum of all elements.
 */
VALUE_TYPE sum_value_array(VALUE_TYPE* array, uint64_t no_elements);

/**
 * @brief Determine how many elements are not zero within an array.
 *
 * @param[in] array Array which shall be analysed.
 * @param[in] no_elements Length of array.
 * @return The number of elements which are not zero.
 */
uint64_t get_nnz_key_array(KEY_TYPE* array, uint64_t no_elements);

/**
 * @brief Calculate an approximate euclidean distance between two vectors using
 *        only their squared lengths.
 *
 * @param[in] vector_one_len_squared Length of first vector squared.
 * @param[in] vector_two_len_squared Length of first vector squared.
 * @return Approximated euclidean distance.
 */
VALUE_TYPE lower_bound_euclid(VALUE_TYPE vector_one_len_squared
                            , VALUE_TYPE vector_two_len_squared);

/**
 * @brief Return the larger value of a/b.
 *
 * @param a First value to compare.
 * @param b Second value to compare.
 * @return Return a if (a > b) else return b.
 */
VALUE_TYPE value_type_max(VALUE_TYPE a, VALUE_TYPE b);

#endif /* COMMON_VECTOR_MATH_H */
