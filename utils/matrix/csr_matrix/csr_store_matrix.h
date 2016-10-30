#ifndef CSR_STORE_MATRIX_H
#define CSR_STORE_MATRIX_H

#include "csr_matrix.h"
#include "../../utstring.h"

/**
 * @brief Convert csr_matrix into a libsvm file.
 *
 * @param[in] mtrx Matrix to convert.
 * @param[in] output_path Path of the result file.
 * @param[in] labels Vector of labels. If this is null static label is used.
 * @param[in] static_label This is used if labels is NULL as a label for all samples.
 * @return 0 if conversion succeeded else 1.
 */
uint32_t store_matrix_with_label(struct csr_matrix *mtrx, int32_t* labels
                                , int32_t static_label, char* output_path);

/**
 * @brief Convert csr_matrix into a libsvm string.
 *
 * @param[in] mtrx Matrix to convert to string.
 * @param[in] labels Vector of labels. If this is null static label is used.
 * @param[in] static_label This is used if labels is NULL as a label for all samples.
 * @return matrix as string.
 */
char* store_matrix_with_label_as_string(struct csr_matrix *mtrx, int32_t* labels
                                       , int32_t static_label);

/**
 * @brief Convert a single sparse vector into libsvm format and store it in a string.
 *
 * @param keys of the sparse sample
 * @param values of the sparse sample
 * @param nnz Number of non zero value of the sparse sample = len(keys).
 * @param label Label of this sample.
 * @param result_string object to store the string.
 */
void create_ascii_lsvm_with_label(KEY_TYPE* keys, VALUE_TYPE* values,
                                  uint64_t nnz, int32_t label,
                                  UT_string *result_string);

#endif /* CSR_STORE_MATRIX_H */
