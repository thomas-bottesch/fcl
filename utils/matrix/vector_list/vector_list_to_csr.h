#ifndef VECTOR_LIST_TO_CSR_H
#define VECTOR_LIST_TO_CSR_H

#include "vector_list.h"
#include "../csr_matrix/csr_matrix.h"

/**
 * @brief Remove vectors from a list of sparse vectors which have nnz=0.
 *
 * @param[in] mtrx The list of sparse vectors to remove empty vectors from.
 * @param[in] no_vectors length of mtrx and mask.
 * @param[in] dim The dimensionality of mtrx.
 * @param[in] mask Array of true false. True means keep vector, false remove vector.
 * @return New csr matrix without empty vectors.
 */
struct csr_matrix* create_matrix_without_empty_elements(struct sparse_vector* mtrx
                                                           , uint64_t no_vectors
                                                           , uint64_t dim
                                                           , uint64_t* mask);

/**
 * Convert a list of sparse vectors into a csr matrix.
 *
 * @param[in] mtrx as array of sparse vectors
 * @param[in] no_vectors length of mtrx array
 * @param[in] dim dimensionality of sparse vectors
 * @param[out] csr_mtrx result matrix in csr format
 */
void sparse_vector_list_to_csr_matrix(struct sparse_vector* mtrx
                                      , uint64_t no_vectors
                                      , uint64_t dim
                                      , struct csr_matrix* csr_mtrx);

#endif /* VECTOR_LIST_TO_CSR_H */
