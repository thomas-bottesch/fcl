#ifndef VECTOR_LIST_H
#define VECTOR_LIST_H

#include "../../types.h"

/**
 * @brief A sparse vector
 *
 */
struct sparse_vector {
    KEY_TYPE *keys;                /**< Keys of the sparse vector. */
    VALUE_TYPE *values;            /**< Values of the sparse vector. */
    uint64_t nnz;                  /**< Length of keys/values. */
};

/**
 * Deallocate the contents of a vector list.
 *
 * @param[in] vec_list List of vectors to cleanup.
 * @param[in] no_vectors Length of vec_list.
 */
void free_vector_list(struct sparse_vector* vec_list, uint64_t no_vectors);

#endif /* VECTOR_LIST_H */
