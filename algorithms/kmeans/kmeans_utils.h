#ifndef KMEANS_UTILS_H
#define KMEANS_UTILS_H

#include "kmeans_control.h"
#include "kmeans_cluster_hashmap.h"
#include <unistd.h>

/**
 * @brief General context has information about the currently running kmeans algorithm
 *        like internal counters which are the same for all k-means algorithms.
 */
struct general_kmeans_context {
    uint64_t total_no_calcs;            /**< total number of full distance calculations */

    /* array of true/false. For every cluster if a cluster did not gain/loose
     * any samples in the last iteration it is set to false.
     */
    uint32_t *clusters_not_changed;

    uint32_t *was_assigned;            /**< For every sample: Was it assigned to any cluster yet? */
    uint64_t *cluster_counts;          /**< Number of samples contained in every cluster */
    uint64_t *cluster_assignments;     /**< For every sample contains the assigned cluster */

    VALUE_TYPE wcssd;                  /**< objective recalculated in every iteration */

    /* contains the objective value of the last iteration if the new objective
     * is is less than a threshold away from this value we stop
     */
    VALUE_TYPE old_wcssd;

    uint64_t no_changes;                    /**< #samples that switched clusters in the last iteration */
    uint64_t done_calculations;             /**< #full distance calculations done in last iteration */

    VALUE_TYPE *cluster_distances;          /**< distance samples to cluster */
    VALUE_TYPE *vector_lengths_samples;     /**< ||s|| for every s in samples */
    VALUE_TYPE *vector_lengths_clusters;    /**< ||c|| for every c in clusters */

    struct csr_matrix *samples;               /**< samples as csr */
    /* struct csr_matrix *clusters; */             /**< cluster centers as csr */
    struct keyvaluecount_hash **clusters_raw; /**< cluster centers as hashmap */

    uint64_t no_clusters;                            /**< length of cluster_vectors / shifted_cluster_vectors array */
    struct sparse_vector* cluster_vectors;           /**< cluster centers as a list of sparse vectors */
    struct sparse_vector* shifted_cluster_vectors;   /**< cluster centers as a list of sparse vectors */

    uint64_t *previous_cluster_assignments; /**< remembering to which cluster a sample was assigned in the last iteration avoids calculating that distance again */

    VALUE_TYPE *vector_lengths_shifted_clusters; /**< ||c|| for every c in clusters after shifting */
    struct csr_matrix *shifted_clusters;         /**< csr matrix of shifted clusters */

    /* time stuff*/
    struct timeval tm_start_iteration;   /**< used to keep track of duration of a complete iter */
    struct timeval tm_start;             /**< used to keep track of overall elapsed time */
    struct timeval durations;            /**< used to keep track of various durations */
    uint32_t track_time;                 /**< enables/disables time tracking */
    VALUE_TYPE duration_all_calcs;          /**< measures time needed to do calculations per iteration */
    VALUE_TYPE duration_update_clusters;    /**< measures time needed to shift clusters */

    /* if fabs(wcssd - old_wcssd) < threshold, this gets set to true */
    uint32_t converged;
};

/**
 * @brief Defines a cluster group
 */
struct group {
    uint64_t* clusters;             /**< Array of cluster ids which are part of this group. */
    uint64_t no_clusters;           /**< Length of clusters array. */
};

struct convergence_context {
    uint32_t initialized;              /**< True if struct was initialized */
    VALUE_TYPE ewa_wcssd;              /**< Exponentially Weighted Average of the wcssd */
    VALUE_TYPE ewa_wcssd_min;          /**< Exponentially Weighted Average of the last improvement on ewa_wcssd */
    uint32_t not_improved_counter;     /**< Counter describing how long ewa_cluster_diff did not improve */
};

/**
 * @brief Calculates for every s \in mtrx the distance to the cluster it is currently
 *        assigned to
 *
 * @param[in] mtrx Matrix of samples
 * @param[in] clusters the cluster centers
 * @param[in] no_clusters length of clusters array
 * @param[in] cluster_assignments for every s \in mtrx the cluster_id s is assigned to
 * @param[in] vector_lengths ||s||² for every s \in mtrx
 * @param[out] cluster_distances for every s \in mtrx || s - c ||²
 *                               where c is taken from cluster_assignments
 */
void calculate_initial_distances_clusters(struct csr_matrix *samples
                                           , struct sparse_vector* clusters
                                           , uint64_t no_clusters
                                           , uint64_t* cluster_assignments
                                           , VALUE_TYPE* vector_lengths_samples
                                           , VALUE_TYPE* cluster_distances);

/**
 * @brief Calculate the distance between cluster centers before and after shifting.
 *
 * Suppose c \in C is a cluster center before shifting.
 * Suppose c' is the cluster center after shifting.
 *
 * @param[out] distance_clustersold_to_clustersnew || c - c' ||² for every c in C
 * @param[in] new_clusters Cluster centers before shifting.
 * @param[in] old_clusters Cluster centers after shifting.
 * @param[in] no_clusters length of new_clusters and old_clusters
 * @param[in] vector_length_new_clusters || c ||² for every c \in C.
 * @param[in] vector_length_old_clusters || c' ||² for every c \in C'.
 * @param[in] clusters_not_changed Array for every center true if they did not change.
 */
void calculate_distance_clustersold_to_clustersnew(VALUE_TYPE* distance_clustersold_to_clustersnew
                                                   , struct sparse_vector* new_clusters
                                                   , struct sparse_vector* old_clusters
                                                   , uint64_t no_clusters
                                                   , VALUE_TYPE* vector_length_new_clusters
                                                   , VALUE_TYPE* vector_length_old_clusters
                                                   , uint32_t* clusters_not_changed);

/**
 *
 * @param[in] ctx is the context of a currently running kmeans algorithm.
 * @param[in] chosen_sample_map Array of True/False True if sample was used in last batch
 * @param[in] max_not_improved_counter No. of iterations allowed without improvement.
 * @param[in] conv_ctx Context tracks variables needed for convergence checks.
 */
void post_process_iteration_minibatch(struct general_kmeans_context* ctx
                                      , uint32_t* chosen_sample_map
                                      , uint32_t max_not_improved_counter
                                      , struct convergence_context* conv_ctx);

/**
 * @brief Used to do all preprocessing needed before an iteration of kmeans
 *        which is common in many k-means algorithms.
 *        Memory allocations and counters are resetted here.
 *
 * @param[in] ctx is the context of a currently running kmeans algorithm.
 */
void pre_process_iteration(struct general_kmeans_context* ctx);

/**
 * @brief Used to do all post processing needed after an iteration of kmeans
 *        which is common in many k-means algorithms.
 *
 * The k-means objective is calculated here and the number of samples which moved
 * between clusters.
 *
 * @param[in] ctx is the context of a currently running kmeans algorithm.
 * @param[in] prms are the parameters, the algorithm was started with
 */
void post_process_iteration(struct general_kmeans_context* ctx, struct kmeans_params *prms);

/**
 * @brief Print the iteration summary if verbose was activated.
 *
 * @param[in] ctx is the context of a currently running kmeans algorithm.
 * @param[in] prms are the parameters, the algorithm was started with
 * @param iteration is the number of iteration which have already passed
 */
void print_iteration_summary(struct general_kmeans_context* ctx, struct kmeans_params *prms, uint32_t iteration);

void initialize_kmeans_random(struct general_kmeans_context* ctx,
                              struct kmeans_params *prms);

void initialize_kmeans_pp(struct general_kmeans_context* ctx,
                              struct kmeans_params *prms);

/**
 * @brief Free old ctx->clusters and replace it with ctx->shifted_clusters.
 *
 * @param[in] ctx is the context of a currently running kmeans algorithm.
 */
void switch_to_shifted_clusters(struct general_kmeans_context* ctx);

/**
 * @brief This function initializes the kmeans context.
 *
 * Stuff that happens here:
 * - Allocate memory for the cluster centers
 * - Determine the initial starting position of the clustering
 * - Calculate for every s in samples ||s||²
 * - Calculate for every c in ctx->clusters ||c||²
 *
 * @param[in] prms are the parameters, the algorithm was started with
 * @param[in] ctx is the context of a currently running kmeans algorithm.
 * @param[in] samples which shall be clustered.
 */
void initialize_general_context(struct kmeans_params *prms
                                , struct general_kmeans_context* ctx
                                , struct csr_matrix* samples);

/**
 * @brief Support function to free all elements of a general_kmeans_context.
 *
 * @param[in] ctx is the context which shall be freed.
 * @param[in] prms are the parameters, the algorithm was started with.
 */
void free_general_context(struct general_kmeans_context* ctx
                         , struct kmeans_params *prms);

/**
 * @brief Creates a csr matrix with only non empty resulting clusters.
 *
 * @param[in] prms are the parameters, the algorithm was started with.
 * @param[in] ctx is the context which shall be freed.
 */
struct csr_matrix* create_result_clusters(struct kmeans_params *prms
                                          , struct general_kmeans_context* ctx);

/**
 * @brief Iteratively searches for a block vector matrix until average nnz of the block
 *        vectors is 0.3 * average nnz of the samples.
 *
 * @param[in] prms are the parameters, the algorithm was started with.
 * @param[in] samples which shall be clustered.
 * @param[in] desired_annz Desired size of the resulting block vector 0 to 100 percent
 * @param[out] block_vectors_samples is the matrix of resulting block vectors
 * @param[out] block_vectors_dim is the dimensionality of the block vectors
 */
void search_samples_block_vectors(struct kmeans_params *prms
                                   , struct csr_matrix* samples
                                   , VALUE_TYPE desired_annz
                                   , struct csr_matrix* block_vectors_samples
                                   , uint64_t *block_vectors_dim);

/**
 * @brief In yinyang the clusters are partitioned into cluster groups.
 *
 * The groups are found
 * by doing a regular k-means on the clusters and setting k=no_groups.
 * The list of groups is returned. Each group contains the number of clusters assigned
 * to it and the id's of the assigned clusters.
 *
 * @param[in] clusters_list which shall be clustered into groups
 * @param[in] no_clusters length of cluster_list
 * @param[in] dim dimensionality of clusters_list
 * @param[out] groups is the result of the clustering containing the groups of clusters.
 * @param[out] no_groups is the length of the groups array.
 */
void create_kmeans_cluster_groups(struct sparse_vector *clusters_list
                                  , uint64_t no_clusters
                                  , uint64_t dim
                                  , struct group** groups
                                  , uint64_t* no_groups);

/**
 * Determine the the new cluster centers after finishing a k-means iteration.
 * The resulting new cluster centers are stored in ctx->shifted_clusters.
 *
 * @param[in] ctx is the context of a currently running kmeans algorithm.
 */
void calculate_shifted_clusters(struct general_kmeans_context* ctx);


/**
 * Determine the the new cluster centers after finishing a minibatch k-means iteration
 * The resulting new cluster centers are stored in ctx->shifted_clusters.
 *
 * @param[in] ctx is the context of a currently running kmeans algorithm.
 * @param[in] active_sample_amp array of true/false with length=ctx->samples->sample_count
 *                              True for every sample that was used in the last iteration.
 */
void calculate_shifted_clusters_minibatch_kmeans(struct general_kmeans_context* ctx
                                                , uint32_t* active_sample_map);

#endif
