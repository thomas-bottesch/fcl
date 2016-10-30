#include "sparse_vector_math.h"
#include <math.h>
#include "../../fcl_logging.h"

VALUE_TYPE dot(KEY_TYPE *keys_vector_one
                                , VALUE_TYPE *values_vector_one
                                , uint64_t non_zero_count_vector_one
                                , KEY_TYPE *keys_vector_two
                                , VALUE_TYPE *values_vector_two
                                , uint64_t non_zero_count_vector_two) {

    VALUE_TYPE result;
    uint64_t next_key_vector_two;
    uint64_t nnz_counter_vector_two;
    uint64_t nnz_counter_vector_one;

    result = 0;
    nnz_counter_vector_two = 0;
    if (non_zero_count_vector_one == 0 || non_zero_count_vector_two == 0) return result;
    next_key_vector_two = keys_vector_two[nnz_counter_vector_two];
    for(nnz_counter_vector_one = 0; nnz_counter_vector_one < non_zero_count_vector_one; nnz_counter_vector_one++) {
        while (nnz_counter_vector_two < (non_zero_count_vector_two - 1)
               && keys_vector_one[nnz_counter_vector_one] > next_key_vector_two) {
            nnz_counter_vector_two = nnz_counter_vector_two + 1;
            next_key_vector_two = keys_vector_two[nnz_counter_vector_two];
        }

        if (keys_vector_one[nnz_counter_vector_one] == next_key_vector_two) {
            result += values_vector_one[nnz_counter_vector_one] * values_vector_two[nnz_counter_vector_two];
        }

    }
    return result;
}

VALUE_TYPE dot_binary_search(KEY_TYPE *keys_vector_one
                                , VALUE_TYPE *values_vector_one
                                , uint64_t non_zero_count_vector_one
                                , KEY_TYPE *keys_vector_two
                                , VALUE_TYPE *values_vector_two
                                , uint64_t non_zero_count_vector_two) {

    VALUE_TYPE result;
    uint64_t length_left_vec1;
    uint64_t length_right_vec1;
    uint64_t length_left_vec2;
    uint64_t length_right_vec2;

    if (non_zero_count_vector_one == 0 || non_zero_count_vector_two == 0) return 0;
    if (non_zero_count_vector_one == 1 && non_zero_count_vector_two == 1) {
        result = 0;
        if (keys_vector_one[0] == keys_vector_two[0]) {
            return values_vector_one[0] * values_vector_two[0];
        }
        return result;
    }
    length_left_vec1 = non_zero_count_vector_one / 2;
    length_right_vec1 = non_zero_count_vector_one - length_left_vec1;
    length_left_vec2 = non_zero_count_vector_two / 2;
    length_right_vec2 = non_zero_count_vector_two - length_left_vec2;

    result = 0;
    if (keys_vector_one[length_left_vec1 - (non_zero_count_vector_one % 2 == 0)]
       < keys_vector_two[length_left_vec2 - (non_zero_count_vector_two % 2 == 0)]) {
        if (non_zero_count_vector_one == 1 && keys_vector_one[0] < keys_vector_two[0]) return 0;
        result += dot_binary_search(keys_vector_one, values_vector_one, length_left_vec1 + (non_zero_count_vector_one % 2)
                      , keys_vector_two, values_vector_two, length_left_vec2 - (non_zero_count_vector_two % 2 == 0));

        result += dot_binary_search(keys_vector_one + length_left_vec1 + (non_zero_count_vector_one % 2), values_vector_one + length_left_vec1 + (non_zero_count_vector_one % 2), length_right_vec1 - (non_zero_count_vector_one % 2)
                      , keys_vector_two, values_vector_two, non_zero_count_vector_two);

    } else if (keys_vector_one[length_left_vec1 - (non_zero_count_vector_one % 2 == 0)]
                               > keys_vector_two[length_left_vec2 - (non_zero_count_vector_two % 2 == 0)]){
        result += dot_binary_search(keys_vector_one, values_vector_one, length_left_vec1 - (non_zero_count_vector_one % 2 == 0)
                    , keys_vector_two, values_vector_two, non_zero_count_vector_two);

        result += dot_binary_search(keys_vector_one + length_left_vec1 - (non_zero_count_vector_one % 2 == 0), values_vector_one + length_left_vec1 - (non_zero_count_vector_one % 2 == 0), length_right_vec1 + (non_zero_count_vector_one % 2 == 0)
                    , keys_vector_two + length_left_vec2 + (non_zero_count_vector_two % 2), values_vector_two + length_left_vec2 + (non_zero_count_vector_two % 2), length_right_vec2 - (non_zero_count_vector_two % 2));
    } else {
        result += values_vector_one[length_left_vec1 - (non_zero_count_vector_one % 2 == 0)]
                  * values_vector_two[length_left_vec2 - (non_zero_count_vector_two % 2 == 0)];

        result += dot_binary_search(keys_vector_one, values_vector_one, length_left_vec1 - (non_zero_count_vector_one % 2 == 0)
                  , keys_vector_two, values_vector_two, length_left_vec2 - (non_zero_count_vector_two % 2 == 0));

        result += dot_binary_search(keys_vector_one + length_left_vec1 + (non_zero_count_vector_one % 2), values_vector_one + length_left_vec1 + (non_zero_count_vector_one % 2), length_right_vec1 - (non_zero_count_vector_one % 2)
                    , keys_vector_two + length_left_vec2 + (non_zero_count_vector_two % 2), values_vector_two + length_left_vec2 + (non_zero_count_vector_two % 2), length_right_vec2 - (non_zero_count_vector_two % 2));

    }

    return result;
}

/*
 * Calc the euclidean distance between two sparse vectors
 * Assumption dimensions of dense matrix == dim_matrix
 *
 */
VALUE_TYPE euclid_vector(KEY_TYPE *keys_vector_one
                        , VALUE_TYPE *values_vector_one
                        , uint64_t non_zero_count_vector_one
                        , KEY_TYPE *keys_vector_two
                        , VALUE_TYPE *values_vector_two
                        , uint64_t non_zero_count_vector_two
                        , VALUE_TYPE vector_one_length_squared
                        , VALUE_TYPE vector_two_length_squared) {

    return sqrt( value_type_max( vector_one_length_squared
                      + vector_two_length_squared
                      - 2
                      * dot(keys_vector_one
                            , values_vector_one
                            , non_zero_count_vector_one
                            , keys_vector_two
                            , values_vector_two
                            , non_zero_count_vector_two)
                 , 0.0 ) ) ;
}

uint64_t get_blockvector_nnz(KEY_TYPE* keys, VALUE_TYPE* values, uint64_t nnz, uint64_t keys_per_block) {
    uint64_t nnz_bv, j;
    uint64_t current_chunk, new_chunk, initialized;
    VALUE_TYPE current_val;

    nnz_bv = 0;
    current_chunk = 0;
    initialized = 0;

    for (j = 0; j < nnz; j++) {
        current_val = pow(values[j], 2.0);
        new_chunk = keys[j] / keys_per_block;
        if (current_val > 0) {
            if (initialized == 0) {
                initialized = 1;
                nnz_bv = 1;
            } else {
                if (current_chunk != new_chunk) nnz_bv++;
            }
            current_chunk = new_chunk;
        }
    }

    return nnz_bv;
}

void fill_blockvector(KEY_TYPE* keys
                      , VALUE_TYPE* values
                      , uint64_t nnz
                      , uint64_t keys_per_block
                      , KEY_TYPE* blockvector_keys
                      , VALUE_TYPE* blockvector_values
                      , uint64_t* bv_nnz) {
    uint64_t j;
    VALUE_TYPE current_val;
    uint64_t current_chunk, new_chunk,  initialized;

    initialized = 0;
    current_chunk = 0;

    for (j = 0; j < nnz; j++) {
        current_val = pow(values[j], 2.0);
        new_chunk = keys[j] / keys_per_block;
        if (current_val > 0) {
            if (initialized == 0) {
                initialized = 1;
                *bv_nnz = 1;
            } else {
                if (current_chunk != new_chunk) (*bv_nnz)++;
            }
            current_chunk = new_chunk;

            blockvector_keys[*bv_nnz - 1] = current_chunk;
            blockvector_values[*bv_nnz - 1] += current_val;
        }
    }

    for (j = 0; j < *bv_nnz; j++) {
        blockvector_values[j] = sqrt(blockvector_values[j]);
        if (blockvector_values[j] == 0) LOG_ERROR("ERROR: Values_Chunk is zero %f", blockvector_values[j]);
    }
}
