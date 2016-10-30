#include "common_vector_math.h"
#include <math.h>

uint64_t get_nnz_uint64_array(uint64_t* array, uint64_t no_elements) {
    uint64_t nnz, j;
    nnz = 0;
    for (j = 0; j < no_elements; j++) {
        if (array[j] != 0) {
            nnz += 1;
        }
    }
    return nnz;
}

VALUE_TYPE calculate_squared_vector_length(VALUE_TYPE *vector, uint64_t dim) {
    VALUE_TYPE squared_vector_length;
    uint64_t iter;
    squared_vector_length = 0;

    for(iter = 0; iter < dim; iter++) {
        squared_vector_length += vector[iter] * vector[iter];
    }
    return squared_vector_length;
}

VALUE_TYPE sum_value_array(VALUE_TYPE* array, uint64_t no_elements) {
    VALUE_TYPE sum;
    uint64_t j;

    sum = 0;
    #pragma omp parallel for reduction(+:sum)
    for (j = 0; j < no_elements; j++) {
        sum += array[j];
    }

    return sum;
}

uint64_t get_nnz_key_array(KEY_TYPE* array, uint64_t no_elements) {
    uint64_t nnz, j;
    nnz = 0;
    for (j = 0; j < no_elements; j++) {
        if (array[j] != 0) {
            nnz += 1;
        }
    }
    return nnz;
}

VALUE_TYPE lower_bound_euclid(VALUE_TYPE vector_one_length_squared
                        , VALUE_TYPE vector_two_length_squared) {

    return sqrt(value_type_max( vector_one_length_squared
            + vector_two_length_squared
            - (2.0 * (sqrt(vector_one_length_squared)
              * sqrt(vector_two_length_squared))), 0.0 ));
}

VALUE_TYPE value_type_max(VALUE_TYPE a, VALUE_TYPE b) {
    return (a > b) ? a : b;
}
