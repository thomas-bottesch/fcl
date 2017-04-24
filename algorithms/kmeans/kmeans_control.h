#include "../../utils/matrix/csr_matrix/csr_matrix.h"
#include "../../utils/cdict.h"

#ifndef KMEANS_CONTROL_H
#define KMEANS_CONTROL_H

#define NO_KMEANS_ALGOS                      UINT32_C(9)
#define ALGORITHM_KMEANS                     UINT32_C(0)
#define ALGORITHM_KMEANS_OPTIMIZED           UINT32_C(1)
#define ALGORITHM_KMEANS_OPTIMIZED_ONDEMAND  UINT32_C(2)
#define ALGORITHM_YINYANG                    UINT32_C(3)
#define ALGORITHM_FAST_YINYANG               UINT32_C(4)
#define ALGORITHM_FAST_YINYANG_ONDEMAND      UINT32_C(5)
#define ALGORITHM_MINIBATCH_KMEANS           UINT32_C(6)
#define ALGORITHM_MINIBATCH_KMEANS_OPTIMIZED UINT32_C(7)
#define ALGORITHM_ELKAN_KMEANS               UINT32_C(8)

#define NO_KMEANS_INITS                      UINT32_C(2)
#define KMEANS_INIT_RANDOM                   UINT32_C(0)
#define KMEANS_INIT_KMPP                     UINT32_C(1)

extern const char *KMEANS_ALGORITHM_NAMES[NO_KMEANS_ALGOS];
extern const char *KMEANS_ALGORITHM_DESCRIPTION[NO_KMEANS_ALGOS];
extern const char *KMEANS_INIT_NAMES[NO_KMEANS_INITS];
extern const char *KMEANS_INIT_DESCRIPTION[NO_KMEANS_INITS];

/**
 * @brief Parameters to control the k-means execution
 */
struct kmeans_params {
    /* algorithm params */
    uint32_t kmeans_algorithm_id;   /**< id specifies which k-means algorithm to use */
    uint32_t no_clusters;           /**< the number of clusters (k in k-means)  */
    uint32_t seed;                  /**< different seeds result in different inits. */
    uint32_t iteration_limit;       /**< stop k-means after this many iterations  */
    VALUE_TYPE tol;                 /**< if objective is less than this, the algorithm converges  */
    uint32_t verbose;               /**< if true print info messages while executing  */
    uint32_t init_id;               /**< id specifies the kmeans init strategy to use  */
    uint32_t remove_empty;          /**< if true removes empty clusters before returning the resulting clusters  */
    uint32_t stop;                  /**< an immediate stop of the alforithm was requested  */
    struct cdict* tr;               /**< tracking data results. e.g. calculations per iteration */
};

typedef struct csr_matrix* (*kmeans_algorithm_function) (struct csr_matrix* samples, struct kmeans_params *prms);
extern kmeans_algorithm_function KMEANS_ALGORITHM_FUNCTIONS[NO_KMEANS_ALGOS];

#endif
