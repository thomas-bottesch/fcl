#include "fcl_kmeans_commons.h"
#include "../../algorithms/kmeans/kmeans.h"
#include "../../algorithms/kmeans/yinyang.h"
#include "../../algorithms/kmeans/minibatch_kmeans.h"
#include "../../algorithms/kmeans/kmeans_control.h"
#include "../../algorithms/kmeans/kmeans_utils.h"
#include "../../utils/fcl_random.h"
#include "../../utils/fcl_logging.h"
#include <stdlib.h>

uint32_t fcl_kmeans_fit(const mxArray* input_data
                   , const  mxArray* k
                   , const  mxArray* opts
                   , struct kmeans_params** prms
                   , struct kmeans_result **res
                   , struct csr_matrix **input_dataset) {


    uint32_t num_clusters;                   // the value of k

    *prms = NULL;
    *res = NULL;
    *input_dataset = NULL;

    *prms = (struct kmeans_params*) calloc(1, sizeof(struct kmeans_params));

    initialize_random_generator();

    // create struct with default parameters;
    (*prms)->kmeans_algorithm_id = ALGORITHM_BV_KMEANS;
    (*prms)->seed = rand() % ((uint32_t) -1);
    (*prms)->iteration_limit = 1000;
    (*prms)->tol = 1e-6;
    (*prms)->verbose = 1;
    (*prms)->init_id = KMEANS_INIT_RANDOM;
    (*prms)->remove_empty=0;
    (*prms)->stop=0;
    (*prms)->tr=NULL;
    (*prms)->initprms=NULL;

    // read optional input parameters if available
    if (opts == NULL) {
#ifdef DEBUG
        LOG_DEBUG("Using default parameters.");
#endif
    } else {
        int successful = read_optional_params(*prms, opts);
        if (!successful) goto fcl_kmeans_fit_error;
    }

    if ((*prms)->initprms == NULL)  {
        // check value of k
        if (mxGetScalar(k) < 1 || mxGetScalar(k) > UINT32_MAX) {
            LOG_ERROR("Wrong input. Number of clusters should be at least 1 and < 2^32 - 1.");
            goto fcl_kmeans_fit_error;
        }
        num_clusters = mxGetScalar(k);
        (*prms)->no_clusters = num_clusters;
    }

#ifdef DEBUG
    if ((*prms)->verbose) {
        LOG_DEBUG("Converting data to internal sparse representation.");
    }
#endif

    if (!load_dataset(input_data, input_dataset)) {
        LOG_ERROR("Error while loading dataset!");
        goto fcl_kmeans_fit_error;
    }

    // print parameters
#ifdef DEBUG
    if ((*prms)->verbose)  {
        LOG_DEBUG("kmeans_algorithm_id = %" PRINTF_INT32_MODIFIER "u", (*prms)->kmeans_algorithm_id);
        LOG_DEBUG("no_clusters = %" PRINTF_INT32_MODIFIER "u", (*prms)->no_clusters);
        LOG_DEBUG("seed = %" PRINTF_INT32_MODIFIER "u", (*prms)->seed);
        LOG_DEBUG("iteration_limit = %" PRINTF_INT32_MODIFIER "u", (*prms)->iteration_limit);
        LOG_DEBUG("tol = %g", (*prms)->tol);
        LOG_DEBUG("verbose = %" PRINTF_INT32_MODIFIER "u", (*prms)->verbose);
        LOG_DEBUG("init_id = %" PRINTF_INT32_MODIFIER "u", (*prms)->init_id);
        LOG_DEBUG("remove_empty = %" PRINTF_INT32_MODIFIER "u", (*prms)->remove_empty);
        LOG_DEBUG("stop = %" PRINTF_INT32_MODIFIER "u", (*prms)->stop);
    }
#endif

    // run algorithm
    *res = KMEANS_ALGORITHM_FUNCTIONS[(*prms)->kmeans_algorithm_id](*input_dataset, *prms);

    return 1;

fcl_kmeans_fit_error:
    if (*prms) {
        free_cdict(&((*prms)->tr));
        free_null(*prms);
    }

    if (*input_dataset) {
        free_csr_matrix(*input_dataset);
        free_null(*input_dataset);
    }

    if (*res) {
        free_kmeans_result(*res);
        *res = NULL;
    }
    return 0;
}
