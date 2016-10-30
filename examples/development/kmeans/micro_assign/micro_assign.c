#include "../../../../utils/matrix/csr_matrix/csr_assign.h"
#include "../../../../utils/matrix/csr_matrix/csr_load_matrix.h"
#include "../../../../utils/matrix/csr_matrix/csr_math.h"
#include "../../../../utils/fcl_logging.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * This example shows how to load a kmeans model in C.
 * The model is then used to determine the closest cluster in the model
 * for an example vector defined through example_keys, example_values.
 * This definition conforms to https://en.wikipedia.org/wiki/Sparse_matrix
 *  - Compressed sparse row (CSR, CRS or Yale format)
 */

int main (int argc, char *argv[]) {
    VALUE_TYPE *vector_lengths_clusters;
    struct csr_matrix *clusters;
    uint64_t closest_cluster;
    VALUE_TYPE closest_distance;

    /* Attention the vector created here is equal to:
     * In libsvm format:
     * <lbl> 1:0.4 3:0.2
     *
     * Keys in the libsvm string are always >= 1. Therefor they are decremented by 1
     * to support a zero based indexing in C.
     */
    KEY_TYPE example_keys[] = {0,2};
    VALUE_TYPE example_values[] = {0.4, 0.2};

    /* create an example model as string (also possible to load the model from disk)
     * This model corresponds to the following dense matrix:
     *   (  0, 0, 0.2
     *    0.4, 0, 0.5)
     *
     * Each row of this matrix is a sample with three dimensions.
     */
    char* input_model = "1       3:0.2\n"
                        "1 1:0.4 3:0.5  ";

    if (convert_libsvm_file_to_csr_matrix_wo_labels(input_model, &clusters)) {
        LOG_ERROR("unable to load input model!");
        exit(-1);
    }

    /* calculate ||c||² for every c in clusters */
    calculate_matrix_vector_lengths(clusters, &vector_lengths_clusters);

    /* Find the closest cluster for our example vector */
    assign_vector(example_keys
                  , example_values
                  , sizeof(example_keys) / sizeof(example_keys[0])  /* nnz */
                  , clusters
                  , vector_lengths_clusters
                  , &closest_cluster
                  , &closest_distance);

    LOG_INFO("Closest cluster: %" PRINTF_INT64_MODIFIER "u with distance %f", closest_cluster, closest_distance);

    free_null(vector_lengths_clusters);
    free_csr_matrix(clusters);
    free_null(clusters);

    return 0;
}
