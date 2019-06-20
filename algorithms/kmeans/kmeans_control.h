#include "../../utils/matrix/csr_matrix/csr_matrix.h"
#include "../../utils/cdict.h"
#include "init_params.h"

#ifndef KMEANS_CONTROL_H
#define KMEANS_CONTROL_H

#define NO_KMEANS_ALGOS                               UINT32_C(19)
#define ALGORITHM_KMEANS                              UINT32_C(0)
#define ALGORITHM_BV_KMEANS                           UINT32_C(1)
#define ALGORITHM_BV_KMEANS_ONDEMAND                  UINT32_C(2)
#define ALGORITHM_YINYANG                             UINT32_C(3)
#define ALGORITHM_BV_YINYANG                          UINT32_C(4)
#define ALGORITHM_BV_YINYANG_ONDEMAND                 UINT32_C(5)
#define ALGORITHM_MINIBATCH_KMEANS                    UINT32_C(6)
#define ALGORITHM_BV_MINIBATCH_KMEANS                 UINT32_C(7)
#define ALGORITHM_PCA_MINIBATCH_KMEANS                UINT32_C(8)
#define ALGORITHM_ELKAN_KMEANS                        UINT32_C(9)
#define ALGORITHM_BV_ELKAN_KMEANS                     UINT32_C(10)
#define ALGORITHM_BV_ELKAN_KMEANS_ONDEMAND            UINT32_C(11)
#define ALGORITHM_PCA_ELKAN_KMEANS                    UINT32_C(12)
#define ALGORITHM_PCA_YINYANG                         UINT32_C(13)
#define ALGORITHM_PCA_KMEANS                          UINT32_C(14)
#define ALGORITHM_KMEANSPP                            UINT32_C(15)
#define ALGORITHM_BV_KMEANSPP                         UINT32_C(16)
#define ALGORITHM_PCA_KMEANSPP                        UINT32_C(17)
#define ALGORITHM_NC_KMEANS                           UINT32_C(18)


#define NO_KMEANS_INITS                      UINT32_C(3)
#define KMEANS_INIT_RANDOM                   UINT32_C(0)
#define KMEANS_INIT_KMPP                     UINT32_C(1)
#define KMEANS_INIT_PARAMS                   UINT32_C(2)

extern const char *KMEANS_ALGORITHM_NAMES[NO_KMEANS_ALGOS];
extern const char *KMEANS_ALGORITHM_DESCRIPTION[NO_KMEANS_ALGOS];
extern const char *KMEANS_INIT_NAMES[NO_KMEANS_INITS];
extern const char *KMEANS_INIT_DESCRIPTION[NO_KMEANS_INITS];

/**
 * @brief The resulting data retrieved from a clustering.
 */
struct kmeans_result {
    struct csr_matrix* clusters;            /**< csr matrix of the resulting cluster centers */
    struct initialization_params* initprms; /**< parameters that contain info about the initialization step of kmeans */   
};

/**
 * @brief Parameters to control the k-means execution
 */
struct kmeans_params {
    /* algorithm params */
    uint32_t kmeans_algorithm_id;           /**< id specifies which k-means algorithm to use */
    uint32_t no_clusters;                   /**< the number of clusters (k in k-means)  */
    uint32_t seed;                          /**< different seeds result in different inits. */
    uint32_t iteration_limit;               /**< stop k-means after this many iterations  */
    VALUE_TYPE tol;                         /**< if objective is less than this, the algorithm converges  */
    uint32_t verbose;                       /**< if true print info messages while executing  */
    uint32_t init_id;                       /**< id specifies the kmeans init strategy to use  */
    uint32_t remove_empty;                  /**< if true removes empty clusters before returning the resulting clusters  */
    uint32_t stop;                          /**< an immediate stop of the alforithm was requested  */
    struct cdict* tr;                       /**< tracking data results. e.g. calculations per iteration */
    struct csr_matrix* ext_vects;           /**< externally supplied vectors */
    struct initialization_params* initprms; /**< parameters that control the initialization step of kmeans */
};

typedef struct kmeans_result* (*kmeans_algorithm_function) (struct csr_matrix* samples, struct kmeans_params *prms);
extern kmeans_algorithm_function KMEANS_ALGORITHM_FUNCTIONS[NO_KMEANS_ALGOS];

#endif
