#ifndef KMEANS_CLUSTER_HASHMAP_H
#define KMEANS_CLUSTER_HASHMAP_H

#include <stdio.h>
#include "../../utils/types.h"
#include "../../utils/matrix/csr_matrix/csr_matrix.h"
#include "../../utils/matrix/vector_list/vector_list.h"
#include "../../utils/uthash.h"

/**
 * @brief Structure is used to efficiently update sparse cluster centers.
 *
 * Suppose S is the set of samples assigned to a cluster c.
 * Then this data structure is a dictionary:
 *
 * feature_0 -> value = sum({ value(feature_0) | feature_0 \in s for all s \in S})
 *              count =   | { value(feature_0) | feature_0 \in s for all s \in S} |
 *
 * Basically the value for feature_0 is summed up over all samples in S which actually
 * have feature_0 set. And count equals the number of samples which had feature_0 set.
 */
struct keyvaluecount_hash {
    KEY_TYPE id;           /**< The id of a specific dimension of a cluster */
    VALUE_TYPE val;        /**< The accumulated value of one dimension of the cluster center */
    uint64_t count;        /**< The number of samples which had these feature set within this cluster */
    UT_hash_handle hh;     /**< ut_hash element which makes this structure hashable */
};

/**
 * @brief Add one sample to a specific cluster dictionary in clusters_raw.
 *
 * @param[in] clusters_raw Dictionary of dictionarys (one for every cluster)
 * @param[in] keys of the sparse sample
 * @param[in] values of the sparse sample
 * @param[in] nnz Number of non zero values (=length of keys/values)
 * @param[in] cluster_id The cluster id to add this sample to.
 * @return True(1) if a new item was added to the dictionary else False(0)
 */
uint32_t add_sample_to_hashmap(struct keyvaluecount_hash** clusters_raw
                                      , KEY_TYPE* keys
                                      , VALUE_TYPE* values
                                      , uint64_t nnz
                                      , uint64_t cluster_id);

/**
 * @brief Add one sample to a specific cluster dictionary in clusters_raw.
 *        for the minibatch kmeans case
 *
 * @param[in] clusters_raw Dictionary of dictionarys (one for every cluster)
 * @param[in] keys of the sparse sample
 * @param[in] values of the sparse sample
 * @param[in] nnz Number of non zero values (=length of keys/values)
 * @param[in] cluster_id The cluster id to add this sample to.
 * @param[in] cluster_count The number of samples that were already added to this cluster.
 * @return True(1) if a new item was added to the dictionary else False(0)
 */
uint32_t add_sample_to_hashmap_minibatch_kmeans(struct keyvaluecount_hash** clusters_raw
                                      , KEY_TYPE* keys
                                      , VALUE_TYPE* values
                                      , uint64_t nnz
                                      , uint64_t cluster_id
                                      , uint64_t cluster_count);

/**
 * @brief Remove one sample from a specific cluster dictionary in clusters_raw.
 *
 * @param[in] clusters_raw Dictionary of dictionarys (one for every cluster)
 * @param[in] keys of the sparse sample
 * @param[in] values of the sparse sample
 * @param[in] nnz Number of non zero values (=length of keys/values)
 * @param[in] cluster_id The cluster id to remove this sample from.
 */
void remove_sample_from_hashmap(struct keyvaluecount_hash** clusters_raw
                                       , KEY_TYPE* keys
                                       , VALUE_TYPE* values
                                       , uint64_t nnz
                                       , uint64_t cluster_id);

/**
 * @brief Comparator function used for sorting.
 *
 * @param[in] a First element which shall be compared.
 * @param[in] b Second element which shall be compared.
 * @return (-1) if a->id < b->id
 *         ( 0) if a->id == b->id
 *         ( 1) if a->id > b->id
 */
int id_sort(struct keyvaluecount_hash *a, struct keyvaluecount_hash *b);


/**
 * @brief Cleanup a specific cluster dictionary/hashmap.
 *
 * @param[in] k Dictionary/Hashmap to cleanup.
 */
void free_keyvaluecount_hashmap(struct keyvaluecount_hash *k);

/**
 * Cleanup all cluster dictionaries/hashmaps.
 *
 * @param[in] clusters The dictionaries to cleanup.
 * @param[in] no_clusters Length of clusters dictionary
 */
void free_cluster_hashmaps(struct keyvaluecount_hash ** clusters
                           , uint64_t no_clusters);

/**
 * Create a csr matrix from a dictionary of cluster dictionaries.
 *
 * @param[in] clusters_raw Dictionary of cluster dictionaries.
 * @param[in] cluster_counts For every cluster dict in clusters_raw the no_samples in that cluster.
 * @param[out] clusters Resulting csr matrix.
 */
void create_matrix_from_hashmap(struct keyvaluecount_hash** clusters_raw
                                       , uint64_t* cluster_counts
                                       , struct csr_matrix *clusters);

/**
 * Create a vector list from a dictionary of cluster dictionaries.
 *
 * @param[in] clusters_raw Dictionary of cluster dictionaries.
 * @param[in] cluster_counts For every cluster dict in clusters_raw the no_samples in that cluster.
 * @param[out] clusters Resulting array of vectors.
 * @param[in] no_cluster length of clusters array.
 */
void create_vector_list_from_hashmap(struct keyvaluecount_hash** clusters_raw
                                       , uint64_t* cluster_counts
                                       , struct sparse_vector *clusters
                                       , uint64_t no_cluster);

#endif
