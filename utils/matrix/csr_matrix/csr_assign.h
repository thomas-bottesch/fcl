#ifndef CSR_ASSIGN_H
#define CSR_ASSIGN_H

#include "csr_matrix.h"

/**
 * @brief Result after assigning an input matrix to a cluster matrix
 */
struct assign_result {
    uint64_t *counts;               /**< number of samples assigned to each cluster */
    uint64_t *assignments;          /**< The closest cluster id for every s in samples */
    VALUE_TYPE *distances;          /**< Distance to the closest cluster id for every s in samples */
    uint64_t len_counts;            /**< Length of the counts array */
    uint64_t len_assignments;       /**< Length of the assignments & distances array */
};

/**
 * @brief Searches for every x in samples the closest vector c in clusters
 *
 * @param[in] samples
 * @param[in] clusters
 * @param[in] stop If the pointer behind this variable gets set, assign will stop immediately.
 * @return The assignment result mainly consists of two arrays: 1. assignments
 *         contains the id of the closest cluster and distances contains the distance
 *         to c for every s.
 */
struct assign_result assign(struct csr_matrix* samples
                           , struct csr_matrix* clusters
                           , uint32_t* stop);

/**
 * @brief Support function to cleanup the assignment result data structure.
 *
 * @param[in] res which shall be cleaned up.
 */
void free_assign_result(struct assign_result* res);

/**
 * @brief Like assign but searches the closest cluster only for one vector instead of
 *        a complete matrix.
 *
 * @param[in] input_keys of the sparse vectors
 * @param[in] input_values corresponding to the keys
 * @param[in] input_non_zero_count_vector
 * @param[in] clusters to be searched
 * @param[in] vector_lengths_clusters contains ||c||² for every c in clusters
 * @param[out] closest_cluster is the output id of closest cluster to the input vector
 * @param[out] closest_cluster_distance the distance to the closest cluster
 */
void assign_vector(KEY_TYPE *input_keys
                   , VALUE_TYPE *input_values
                   , uint64_t input_non_zero_count_vector
                   , struct csr_matrix* clusters
                   , VALUE_TYPE *vector_lengths_clusters
                   , uint64_t* closest_cluster
                   , VALUE_TYPE* closest_cluster_distance);

/**
 * Write the assign result to a csv file.
 *
 * @param[in] res is the assign_result which shall be written to file
 * @param[in] output_path to write to
 * @return 0 if assign result was successfully written to file else 1.
 */
unsigned int store_assign_result(struct assign_result *res, char* output_path);



#endif /* CSR_ASSIGN_H */

