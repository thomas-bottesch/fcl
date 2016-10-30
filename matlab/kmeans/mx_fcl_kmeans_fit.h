#ifndef MX_FCL_KMEANS_FIT_H
#define MX_FCL_KMEANS_FIT_H

#include "mex.h"

#ifdef MATLAB_MEX_FILE
#include "matrix.h"
#endif

#include "../../algorithms/kmeans/kmeans_control.h"

uint32_t fcl_kmeans_fit(const mxArray* input_data
                   , const  mxArray* k
                   , const  mxArray* opts
                   , struct kmeans_params** prms
                   , struct csr_matrix **clusters
                   , struct csr_matrix **input_dataset);

#endif
