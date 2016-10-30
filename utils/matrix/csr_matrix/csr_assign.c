#include <stdio.h>
#include <stdlib.h>
#include "csr_assign.h"
#include "csr_math.h"
#include "../../vector/common/common_vector_math.h"
#include "../../vector/sparse/sparse_vector_math.h"

struct assign_result assign(struct csr_matrix* samples
                           , struct csr_matrix* clusters
                           , uint32_t* stop) {

    struct assign_result res;
    uint64_t sample_id;

    VALUE_TYPE *vector_lengths_clusters;    /* ||c|| for every c in clusters */

    /* calculate ||c|| for every c in clusters */
    calculate_matrix_vector_lengths(clusters, &vector_lengths_clusters);

    res.assignments = (uint64_t*) calloc(samples->sample_count, sizeof(uint64_t));
    res.distances = (VALUE_TYPE*) calloc(samples->sample_count, sizeof(VALUE_TYPE));
    res.counts = (uint64_t*) calloc(clusters->sample_count, sizeof(uint64_t));
    res.len_counts = clusters->sample_count;
    res.len_assignments = samples->sample_count;

    #pragma omp parallel for
    for (sample_id = 0; sample_id < samples->sample_count; sample_id++) {
        res.distances[sample_id] = VALUE_TYPE_MAX;

        if (omp_get_thread_num() == 0) {
            check_signals(stop);
        }

        if (!(*stop)) {
            /* assign every sample to its closest cluster center */
            /* assign samples to cluster centers with given vector lengths */
            assign_vector(samples->keys + samples->pointers[sample_id]
                         , samples->values + samples->pointers[sample_id]
                         , samples->pointers[sample_id + 1] - samples->pointers[sample_id]
                         , clusters
                         , vector_lengths_clusters
                         , res.assignments + sample_id
                         , res.distances + sample_id);

            /* increment the count of the closest cluster */
            #pragma omp critical
            {
                res.counts[res.assignments[sample_id]] += 1;
            }
        }
    }

    free_null(vector_lengths_clusters);
    return res;
}

void assign_vector(KEY_TYPE *input_keys
                   , VALUE_TYPE *input_values
                   , uint64_t input_non_zero_count_vector
                   , struct csr_matrix* clusters
                   , VALUE_TYPE *vector_lengths_clusters
                   , uint64_t* closest_cluster
                   , VALUE_TYPE* closest_cluster_distance) {

    uint64_t cluster_id;
    VALUE_TYPE input_vector_length;
    input_vector_length = calculate_squared_vector_length(input_values
                                                        , input_non_zero_count_vector);

    *closest_cluster_distance = DBL_MAX;

    for (cluster_id = 0; cluster_id < clusters->sample_count; cluster_id++) {
        VALUE_TYPE dist;

        dist =  euclid_vector(input_keys, input_values, input_non_zero_count_vector
                     , clusters->keys + clusters->pointers[cluster_id]
                     , clusters->values + clusters->pointers[cluster_id]
                     , clusters->pointers[cluster_id + 1] - clusters->pointers[cluster_id]
                     , input_vector_length
                     , vector_lengths_clusters[cluster_id]);

        if (dist < *closest_cluster_distance) {
            *closest_cluster = cluster_id;
            *closest_cluster_distance = dist;
        }
    }
}

void free_assign_result(struct assign_result* res) {
    free_null(res->assignments);
    free_null(res->distances);
    free_null(res->counts);
}

uint32_t store_assign_result(struct assign_result *res, char* output_path) {
    uint64_t i;
    FILE* file;

    file = fopen(output_path, "w");
    if (!file) {
        return 1;
    }

    for (i = 0; i < res->len_assignments; i++) {
        fprintf(file, "%" PRINTF_INT64_MODIFIER "u,%.18f", res->assignments[i], res->distances[i]);
        fprintf(file, "\n");
    }

    fclose(file);
    return 0;
}
