#include "csr_load_matrix.h"
#include "../../fcl_file.h"
#include "../../fcl_logging.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

/**
 * @brief Read line from input.
 *
 * @param[in] input
 * @param[out] line
 * @param[out] max_line_len
 * @return
 */
char* readline(FILE *input, char** line, size_t* max_line_len);

/**
 * @brief Read line from input.
 *
 * @param[in] input_str String which shall be read line by line
 * @param[in/out] offset Starting position of the next line
 * @param[out] line Buffer which contains the read string.
 * @param[in/out] max_line_len Maximum number of bytes line can hold.
 * @return
 */
size_t readline_string(const char* input_str, uint64_t *offset, char** line, size_t* max_line_len);

char* readline(FILE *input, char** line, size_t* max_line_len) {
    size_t len;

    if(fgets(*line, *max_line_len, input) == NULL)
        return NULL;

    while(strrchr(*line, '\n') == NULL)
    {
        *max_line_len *= 2;
        *line = (char *) realloc(*line,*max_line_len);
        len = (int) strlen(*line);
        if(fgets(*line + len,*max_line_len - len,input) == NULL)
            break;
    }
    return *line;
}

size_t readline_string(const char* input_str, uint64_t *offset, char** line, size_t* max_line_len) {
    size_t len;
    uint32_t line_feed_found;
    len = 0;
    line_feed_found = 0;

    while (1) {
        while (len < *max_line_len) {
            if ((input_str[*offset + len] == '\0')
                || (input_str[*offset + len] == '\n')) {

                (*line)[len] = '\0';

                if (input_str[*offset + len] == '\n') line_feed_found = 1;
                goto readline_end;
            }
            (*line)[len] = input_str[*offset + len];
            len += 1;
        }

        *max_line_len *= 2;
        *line = (char *) realloc(*line,*max_line_len);
    }

readline_end:
    *offset += len;
    /* continue after the linefeed when calling again */
    if (line_feed_found) (*offset)++;
    return len;
}

uint32_t convert_libsvm_file_to_csr_matrix_wo_labels(const char *input_string, struct csr_matrix **mtrx) {
    int32_t* labels;
    uint32_t status;

    status = convert_libsvm_file_to_csr_matrix(input_string, mtrx, &labels);
    free_null(labels);

    return status;
}

uint32_t convert_libsvm_file_to_csr_matrix(const char *input_string, struct csr_matrix **mtrx, int32_t** labels) {
    uint64_t nnz, i, no_samples;
    KEY_TYPE key_id;
    int64_t inst_max_index;
    FILE *fp;
    char *endptr;
    char *idx, *val, *label;
    char *line = NULL;
    size_t max_line_len;
    uint32_t status;
    uint32_t is_file;
    uint64_t offset;

    *labels = NULL;
    *mtrx = NULL;
    fp = NULL;

    status = 0;
    offset = 0;
    is_file = exists(input_string);

    if (is_file) {

        fp = fopen(input_string,"r");
        if(fp == NULL) {
            LOG_ERROR("can't open input file %s",input_string);
            status = 1;
            goto error;
        }
    }

    no_samples = 0;
    nnz = 0;

    max_line_len = 1024;
    line = (char*) malloc(sizeof(char) * max_line_len);
    while(1){
        char *p;
        if (is_file) {
            if (readline(fp, &line, &max_line_len) == NULL) break;
        } else {
            /* check if end of string reached */
            if (input_string[offset] == '\0') break;
            readline_string(input_string, &offset, &line, &max_line_len);
        }

        p = strtok(line," \t");

        while(1) {
            p = strtok(NULL," \t");
            if(p == NULL || *p == '\n')
                break;
            ++nnz;
        }
        ++no_samples;
    }

    if (is_file) {
        /* rewind to the beginning of the file */
        rewind(fp);
    } else {
        /* set offset to the beginning of the string */
        offset = 0;
    }

    *mtrx = (struct csr_matrix*) malloc(sizeof(struct csr_matrix));
    (*mtrx)->pointers = NULL;
    (*mtrx)->values = NULL;
    (*mtrx)->keys = NULL;
    (*mtrx)->sample_count = 0;
    (*mtrx)->dim = 0;

    (*mtrx)->keys = (KEY_TYPE *) malloc(nnz * sizeof(KEY_TYPE));
    (*mtrx)->values = (VALUE_TYPE *) malloc(nnz * sizeof(VALUE_TYPE));
    (*mtrx)->sample_count = no_samples;
    (*mtrx)->pointers = (POINTER_TYPE *) malloc(((*mtrx)->sample_count + 1) * sizeof(POINTER_TYPE));
    (*mtrx)->pointers[0] = 0;

    *labels = (int32_t *) malloc((*mtrx)->sample_count * sizeof(int32_t));

    key_id = 0;
    for(i=0;i < no_samples;i++) {
        inst_max_index = -1;

        if (is_file) {
            readline(fp, &line, &max_line_len);
        } else {
            readline_string(input_string, &offset, &line, &max_line_len);
        }

        label = strtok(line," \t\n");
        if(label == NULL) {
            LOG_ERROR("invalid libsvm data. label missing in line %ld", i + 1);
            status = 1;
            goto error;
        }

        errno = 0;
        (*labels)[i] = (int32_t) strtol(label, &endptr, 10);

        /* Check for various possible errors */
        if (errno != 0 || endptr == label || *endptr != '\0') {
            LOG_ERROR("invalid libsvm data. invalid label (errno=%d) in line %ld\n", errno, i + 1);
            status = 1;
            goto error;
        }

        while(1) {
            idx = strtok(NULL,":");
            val = strtok(NULL," \t");

            if(val == NULL)
                break;

            errno = 0;
            (*mtrx)->keys[key_id] = (KEY_TYPE) (strtoul(idx, &endptr, 10) - 1);
            if(endptr == idx || errno != 0 || *endptr != '\0' || (*mtrx)->keys[key_id] <= inst_max_index){
                LOG_ERROR("invalid libsvm data. keys invalid or not sorted (errno=%d) in line %ld\n!",errno , i + 1);
                status = 1;
                goto error;
            } else {
                inst_max_index = (*mtrx)->keys[key_id];
            }

            if ((*mtrx)->keys[key_id] + 1 > (*mtrx)->dim) {
                (*mtrx)->dim = (*mtrx)->keys[key_id] + 1;
            }

            errno = 0;
            (*mtrx)->values[key_id] = (VALUE_TYPE) strtod(val,&endptr);
            if(endptr == val || errno != 0 || (*endptr != '\0' && !isspace((unsigned char) *endptr))){
                LOG_ERROR("invalid libsvm data. error while reading values! line: %ld\n %s Key:%" KEY_TYPE_PRINTF_MODIFIER "u", i + 1, endptr, key_id);
                status = 1;
                goto error;
            }

            ++key_id;
        }

        (*mtrx)->pointers[i + 1] = key_id;
    }

error:
    free_null(line);
    if (is_file && fp) fclose(fp);
    if (status != 0) {
        /* some error occured */
        if (*mtrx != NULL) {
            free_csr_matrix(*mtrx);
            free_null(*mtrx);
        }
        if (*labels != NULL) {
            free_null(*labels);
        }
    }

    return status;
}
