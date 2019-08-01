#ifndef FCL_KMEANS_COMMONS_H
#define FCL_KMEANS_COMMONS_H

#include "mex.h"

#ifdef MATLAB_MEX_FILE
#include "matrix.h"
#endif

#include "../../algorithms/kmeans/kmeans_control.h"

#ifdef MATLAB_MEX_FILE

#ifdef __cplusplus
    extern "C" bool utIsInterruptPending();
#else
    extern bool utIsInterruptPending();
#endif

#endif

#define CONVERSION_NUMERIC UINT32_C(0)
#define CONVERSION_STRING  UINT32_C(1)

mxArray* create_struct(struct cdict** d);
uint32_t read_optional_params(struct kmeans_params * prms, const mxArray *entryStruct);
void convert_to_csr_matrix(struct csr_matrix **mtrx, double * X, mwIndex * irs, mwIndex * jcs, mwSize nnz, mwSize num, mwSize dim);
void convert_to_matlab_csc_matrix(struct csr_matrix **clusters, double * X, mwIndex * irs, mwIndex * jcs, uint64_t num, uint64_t nnz );
mxArray* convert_uint64_array_to_mxarray(uint64_t* arr, uint64_t len);
mxArray* convert_valuetype_array_to_mxarray(VALUE_TYPE* arr, uint64_t len);
uint32_t load_dataset(const mxArray* input_data, struct csr_matrix **input_dataset);
mxArray* create_init_params_struct(struct initialization_params* initprms);
#endif
