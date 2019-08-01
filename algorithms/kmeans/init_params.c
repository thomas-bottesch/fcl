#include "init_params.h"
#include <stdlib.h>
#include <errno.h>
#include "../../utils/fcl_file.h"
#include "../../utils/global_defs.h"
#include "../../utils/fcl_logging.h"
#include "../../utils/jsmn.h"

void write_initialization_params_file(const char* fname,
                                      struct initialization_params* initprms) {
    FILE *file;
    uint32_t indent, with_ending_comma;
    LOG_INFO("Inside write_initialization_params_file.");
    if (initprms == NULL) return;
    if (initprms->len_assignments == 0) return;
    if (initprms->assignments == NULL) return;
    if (initprms->len_initial_cluster_samples == 0) return;
    if (initprms->initial_cluster_samples == NULL) return;
    LOG_INFO("Writing file.");
    indent = 4;
    with_ending_comma = 1;
    file = fopen(fname, "w");
    fprintf(file, "{\n");
    json_dump_uint64_array_to_open_file(file,
                                          indent,
                                          TOKEN_INITIAL_CLUSTER_SAMPLES,
                                          initprms->len_initial_cluster_samples,
                                          initprms->initial_cluster_samples,
                                          with_ending_comma);

    with_ending_comma = 0;
    json_dump_uint64_array_to_open_file(file,
                                          indent,
                                          TOKEN_ASSIGNMENTS,
                                          initprms->len_assignments,
                                          initprms->assignments,
                                          with_ending_comma);
    fprintf(file, "}\n");
    fclose(file);
}

int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING &&
        (int) strlen(s) == tok->end - tok->start &&
         strncmp(json + tok->start, s,
                 tok->end - tok->start) == 0) {

        return 0;
    }
    return -1;
}

void parse_json(char* s,
                uint64_t slen,
                jsmntok_t** tokens,
                uint64_t* no_tokens) {
    uint64_t i, comma_count, estimated_tokens;
    int32_t r;
    jsmn_parser p;
    if (s == NULL || slen == 0) return;

    /* the comma count is used to estimate the number of json tokens */
    comma_count = 0;
    for(i = 0; i < slen; i++) {
        if (s[i] == 'c') comma_count++;
    }
    *no_tokens = 0;
    estimated_tokens = (comma_count * 2) + 1;

    while (1) {

        *tokens = (jsmntok_t*) calloc(estimated_tokens, sizeof(jsmntok_t));

        if (*tokens == NULL) return;

        jsmn_init(&p);
        r = jsmn_parse(&p, s, slen, *tokens, estimated_tokens);
        if (r < 0) {
            free_null(*tokens);
            if (r == JSMN_ERROR_NOMEM) {
                estimated_tokens *= 2;
                continue;
            }
            return;
        } else {
            *no_tokens = r;
            break;
        }

    }
}

uint64_t parse_json_token_uint64(char* s, uint64_t slen) {
    uint64_t val;
    char* endptr;
    char* valid_c_string;
    val = 0;

    valid_c_string = malloc(slen + 1);
    memcpy (valid_c_string, s, slen);
    valid_c_string[slen] = 0;

    errno = 0;
    val = strtoul(valid_c_string, &endptr, 10);

    /* if (errno != 0) */
    // conversion failed (EINVAL, ERANGE)

    /* conversion failed (no characters consumed) */
    if (valid_c_string == endptr) errno = 1;
    /* conversion failed (trailing data) */
    if (*endptr != 0)  errno = 1;
    free(valid_c_string);

    return val;
}

void parse_initialization_params_json(char* s,
                                      uint64_t slen,
                                      jsmntok_t* tokens,
                                      uint64_t no_tokens,
                                      struct initialization_params** initprms) {
    uint64_t i, j;
    errno = 0;
    /* Assume the top-level element is an object */
    if (no_tokens < 1 || tokens[0].type != JSMN_OBJECT) {
        LOG_ERROR("Read json tokens %" PRINTF_INT64_MODIFIER "u root type = %d",
                  no_tokens, tokens[0].type);
        return;
    }

    *initprms =  (struct initialization_params*) calloc(1, sizeof(struct initialization_params));

    /* Loop over all keys of the root object */
    for (i = 1; i < no_tokens && errno == 0; i++) {
        if (jsoneq(s, &tokens[i], TOKEN_INITIAL_CLUSTER_SAMPLES) == 0) {
            if (tokens[i+1].type != JSMN_ARRAY) {
                continue; /* We expect groups to be an array of strings */
            }
            (*initprms)->len_initial_cluster_samples = tokens[i+1].size;
            LOG_INFO("len_initial_cluster_samples  %" PRINTF_INT64_MODIFIER "u", (*initprms)->len_initial_cluster_samples);
            (*initprms)->initial_cluster_samples = (uint64_t*) calloc((*initprms)->len_initial_cluster_samples, sizeof(uint64_t));
            for (j = 0; j < (*initprms)->len_initial_cluster_samples && errno == 0; j++) {
                jsmntok_t *g = &tokens[i+j+2];
                (*initprms)->initial_cluster_samples[j] = parse_json_token_uint64(s + g->start, g->end - g->start);
            }
            i += tokens[i+1].size + 1;
        } else if (jsoneq(s, &tokens[i], TOKEN_ASSIGNMENTS) == 0) {
            if (tokens[i+1].type != JSMN_ARRAY) {
                continue; /* We expect groups to be an array of strings */
            }
            (*initprms)->len_assignments = tokens[i+1].size;
            LOG_INFO("len_assignments  %" PRINTF_INT64_MODIFIER "u", (*initprms)->len_assignments);
            (*initprms)->assignments = (uint64_t*) calloc((*initprms)->len_assignments, sizeof(uint64_t));
            for (j = 0; j < (*initprms)->len_assignments && errno == 0; j++) {
                jsmntok_t *g = &tokens[i+j+2];
                (*initprms)->assignments[j] = parse_json_token_uint64(s + g->start, g->end - g->start);
            }
            i += tokens[i+1].size + 1;
        }
    }

    if (errno != 0) {
        free_init_params(*initprms);
        free_null(*initprms);
    }
    return;
}

void read_initialization_params_file(const char* fname,
                                     struct initialization_params** initprms) {
    FILE *file;
    uint64_t no_tokens, fsize;
    char *s;
    size_t result;

    jsmntok_t* tokens; /* We expect no more than 128 tokens */
    *initprms = NULL;
    tokens = NULL;
    s = NULL;

    file = fopen(fname, "r");
    if (file == NULL) {
        LOG_ERROR("Unable to open file %s.", fname);
        return;
    }

    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    s = malloc(fsize + 1);
    if (s == NULL) {
        LOG_ERROR("Unable to malloc for contents of %s.", fname);
        fclose(file);
        return;
    }
    result = fread(s, 1, fsize, file);
    fclose(file);
    s[fsize] = 0;
    if (result != fsize) {
        LOG_ERROR("Read %zu from file should have been %" PRINTF_INT64_MODIFIER "u for %s", result, fsize, fname);
        goto cleanup;
    }

    parse_json(s, fsize, &tokens, &no_tokens);
    if (no_tokens == 0) return;
    parse_initialization_params_json(s, fsize, tokens, no_tokens, initprms);
cleanup:
    free_null(tokens);
    free_null(s);
}

void free_init_params(struct initialization_params* initprms) {
    if (initprms != NULL) {
        free_null(initprms->assignments);
        free_null(initprms->initial_cluster_samples);
    }
}
