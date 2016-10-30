#include "csr_matrix.h"

/**
 * @brief Convert a file in libsvm format into a csr matrix and a labels array.
 *
 * @param[in] filename Path to file in libsvm format.
 * @param[out] mtrx Resulting csr matrix
 * @param[out] labels Labels of the csr matrix.
 * @return 0 if conversion succeeded else 1.
 */
uint32_t convert_libsvm_file_to_csr_matrix(const char *filename
                                       , struct csr_matrix **mtrx
                                       , int32_t** labels);

/**
 * @brief Convert a file in libsvm format into a csr matrix.
 *
 * @param[in] filename Path to file in libsvm format.
 * @param[out] mtrx Resulting csr matrix
 * @return 0 if conversion succeeded else 1.
 */
uint32_t convert_libsvm_file_to_csr_matrix_wo_labels(const char *input_string
                                                     , struct csr_matrix **mtrx);
