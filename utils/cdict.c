#include "cdict.h"

#include "fcl_logging.h"

#define VERBOSE_LOGGING  UINT32_C(0)
#define AUTOCAST_DEFAULT UINT32_C(1)

const char *CDICT_TYPE_AS_STR[] = {"integer"
                                  , "float"
                                  , "dict"
                                  , "dict_list"
                                  , "float_list"
                                  , "integer_list"
                                  , "string"
                                  , "string_list"};

#define ALLOC_CHUNKSIZE 100
#define free_null(ptr) if (ptr) {free(ptr); ptr = NULL;}

char *_cdict_dupstr(const char *s) {
    char *p = malloc(strlen(s) + 1);
    if(p != NULL) strcpy(p, s);
    return p;
}

void _dump_dict_as_json_to_file(struct cdict** d, FILE *file, int32_t indent, int32_t is_named_dict);

void* realloc_zero(void* pBuffer, size_t oldSize, size_t newSize) {
  void* pNew = realloc(pBuffer, newSize);
  if ( newSize > oldSize && pNew ) {
    size_t diff = newSize - oldSize;
    void* pStart = ((char*)pNew) + oldSize;
    memset(pStart, 0, diff);
  }
  return pNew;
}

void free_cdict_element(struct cdict* element) {
    uint64_t i;

    free_null(element->name);

    switch (element->type) {
        case DICT_I:
            /* nothing to free */
            break;
        case DICT_F:
            /* nothing to free */
            break;
        case DICT_D:
            free_cdict(&(element->d_val));
            break;
        case DICT_DL:
            for (i = 0; i < element->val; i++) {
                free_cdict(&(element->d_list[i]));
            }
            free_null(element->d_list);
            break;
        case DICT_FL:
            free_null(element->fval_list);
            break;
        case DICT_IL:
            free_null(element->val_list);
            break;
        case DICT_ST:
            free_null(element->sval);
            break;
        case DICT_SL:
            for (i = 0; i < element->val; i++) {
                free_null(element->sval_list[i]);
            }
            free_null(element->sval_list);
            break;

        default: break;
    }
}

void free_cdict(struct cdict** d) {
    struct cdict *element, *tmp;
    element = NULL;
    tmp = NULL;

    /* free the hash table contents */
    HASH_ITER(hh, *d, element, tmp) {
      HASH_DEL(*d, element);
      free_cdict_element(element);
      free_null(element);
    }
}

void _del_from_cdict(struct cdict** d, struct cdict** element) {
    HASH_DEL( *d, *element);
    free_cdict_element(*element);
    free_null(*element);
}

void delete_cdict_element(struct cdict** d, char* name) {
    struct cdict *element;
    element = NULL;
    HASH_FIND_STR( *d, name, element);
    if (element == NULL) return; /* no element with this name in array */
    _del_from_cdict(d, &element);
}

struct cdict* _check_type_and_create(struct cdict** d, char* name
                                    , uint32_t type, uint32_t verbose) {
    struct cdict *element;
    element = NULL;
    HASH_FIND_STR( *d, name, element);
    if (element != NULL && element->type != type) {
        if (verbose) {
            LOG_WARNING("cdict.c: WARNING removing dict item %s with type=%s and replacing with type=%s!"
                   , name
                   , CDICT_TYPE_AS_STR[element->type]
                   , CDICT_TYPE_AS_STR[type]);
        }
        _del_from_cdict(d, &element);
    }

    if (element == NULL) {
        element = (struct cdict*) calloc(1, sizeof(struct cdict));
        element->name = _cdict_dupstr(name);
        element->type = type;
        HASH_ADD_KEYPTR( hh, *d, element->name, strlen(element->name), element );
    }

    return element;
}

void d_add_int(struct cdict** d, char* name, uint64_t val) {
    struct cdict *element;
    element = _check_type_and_create(d, name, DICT_I, VERBOSE_LOGGING);
    element->val = val;
}

void d_add_float(struct cdict** d, char* name, VALUE_TYPE val) {
    struct cdict *element;
    element = _check_type_and_create(d, name, DICT_F, VERBOSE_LOGGING);
    element->fval = val;
}

void d_add_str(struct cdict** d, char* name, char* sval) {
    struct cdict *element;
    element = _check_type_and_create(d, name, DICT_ST, VERBOSE_LOGGING);
    if (element->sval) free_null(element->sval);
    element->sval = _cdict_dupstr(sval);
}

uint64_t check_and_extend_array(struct cdict* element) {
    struct cdict** new_d_list;
    VALUE_TYPE* new_fval_list;
    uint64_t* new_val_list;
    char** new_sval_list;

    if (element->val < element->alloced) {
        element->val += 1;
        return 1;
    }

    switch (element->type) {
        case DICT_DL:
            if (element->alloced == 0) {
                new_d_list = (struct cdict**) calloc(ALLOC_CHUNKSIZE, sizeof(struct cdict*));
                if (new_d_list == NULL) {
                    return 0;
                }
            } else {
                new_d_list = (struct cdict**) realloc_zero(element->d_list
                									      , element->alloced * sizeof(struct cdict*)
                									      , (element->alloced + ALLOC_CHUNKSIZE) * sizeof(struct cdict*));
                if (new_d_list == NULL) {
                    return 0;
                }
            }

            element->d_list = new_d_list;
            break;
        case DICT_FL:
            if (element->alloced == 0) {
                new_fval_list = (VALUE_TYPE*) calloc(ALLOC_CHUNKSIZE, sizeof(VALUE_TYPE));
                if (new_fval_list == NULL) {
                    return 0;
                }
            } else {
                new_fval_list = (VALUE_TYPE*) realloc_zero(element->fval_list
                										   , element->alloced * sizeof(VALUE_TYPE)
                										   , (element->alloced + ALLOC_CHUNKSIZE) * sizeof(VALUE_TYPE));
                if (new_fval_list == NULL) {
                    return 0;
                }
            }

            element->fval_list = new_fval_list;
            break;
        case DICT_IL:
            if (element->alloced == 0) {
                new_val_list = (uint64_t*) calloc(ALLOC_CHUNKSIZE, sizeof(uint64_t));
                if (new_val_list == NULL) {
                    return 0;
                }
            } else {
                new_val_list = (uint64_t*) realloc_zero(element->val_list
                								        , element->alloced * sizeof(uint64_t)
                									    , (element->alloced + ALLOC_CHUNKSIZE) * sizeof(uint64_t));
                if (new_val_list == NULL) {
                    return 0;
                }
            }

            element->val_list = new_val_list;
            break;
        case DICT_SL:
            if (element->alloced == 0) {
                new_sval_list = (char**) calloc(ALLOC_CHUNKSIZE, sizeof(char*));
                if (new_sval_list == NULL) {
                    return 0;
                }
            } else {
                new_sval_list = (char**) realloc_zero(element->sval_list
                									 , element->alloced * sizeof(char*)
                									 , (element->alloced + ALLOC_CHUNKSIZE) * sizeof(char*));
                if (new_sval_list == NULL) {
                    return 0;
                }
            }

            element->sval_list = new_sval_list;
            break;

        default: break;
    }

    element->alloced += ALLOC_CHUNKSIZE;
    element->val += 1;
    return 1;
}

void d_add_ilist(struct cdict** d, char* name, uint64_t val) {
    struct cdict *element;
    element = _check_type_and_create(d, name, DICT_IL, VERBOSE_LOGGING);
    if (check_and_extend_array(element)) element->val_list[element->val - 1] = val;
}

void d_add_flist(struct cdict** d, char* name, VALUE_TYPE fval) {
    struct cdict *element;
    element = _check_type_and_create(d, name, DICT_FL, VERBOSE_LOGGING);
    if (check_and_extend_array(element)) element->fval_list[element->val - 1] = fval;
}

void d_add_slist(struct cdict** d, char* name, char* str) {
    struct cdict *element;
    element = _check_type_and_create(d, name, DICT_SL, VERBOSE_LOGGING);
    if (check_and_extend_array(element)) element->sval_list[element->val - 1] = _cdict_dupstr(str);
}

struct cdict** d_add_cdict(struct cdict** d, char* name) {
    struct cdict *element;
    element = _check_type_and_create(d, name, DICT_D, VERBOSE_LOGGING);
    return &(element->d_val);
}

void d_add_subint(struct cdict** d, char* subdict_name, char* name, uint64_t val) {
    struct cdict *element;
    element = _check_type_and_create(d, subdict_name, DICT_D, VERBOSE_LOGGING);
    d_add_int(&(element->d_val), name, val);
}

void d_add_subfloat(struct cdict** d, char* subdict_name, char* name, VALUE_TYPE fval) {
    struct cdict *element;
    element = _check_type_and_create(d, subdict_name, DICT_D, VERBOSE_LOGGING);
    d_add_float(&(element->d_val), name, fval);
}

void d_add_substring(struct cdict** d, char* subdict_name, char* name, char* sval) {
    struct cdict *element;
    element = _check_type_and_create(d, subdict_name, DICT_D, VERBOSE_LOGGING);
    d_add_str(&(element->d_val), name, sval);
}

uint64_t d_get_subint_default(struct cdict** d, char* subdict_name
                             , char* name, uint64_t default_val) {
    struct cdict *sub_dict, *element;
    sub_dict = _check_type_and_create(d, subdict_name, DICT_D, VERBOSE_LOGGING);

    element = NULL;
    HASH_FIND_STR(sub_dict->d_val, name, element);
    if (element == NULL || element->type != DICT_I) {
        /*
         * element does either not exist or has the wrong type.
         * Create a new / Overwrite the existing dict element with the default value.
         */

        if (element != NULL && element->type == DICT_F && AUTOCAST_DEFAULT) {
            uint64_t float_as_int;
            /*
             * element is a float lets try to convert it to int
             * we will only do this if it does not change the value
             * else we just use the default!
             */
            float_as_int = (uint64_t) element->fval;
            if (!(float_as_int < element->fval)
                && !(float_as_int > element->fval)) {
                /* equality is ensured! */
                default_val = float_as_int;
            } else {
                LOG_WARNING("Warning: unable to cast dict['%s']['%s'] from float to"
                       " unsigned integer without changing its value. Value was %f but using"
                       " default (%" PRINTF_INT64_MODIFIER "u) instead!\n"
                       , subdict_name, name, element->fval, default_val);
            }
        }

        element = _check_type_and_create(&(sub_dict->d_val), name, DICT_I, VERBOSE_LOGGING);
        element->val = default_val;
    }

    return element->val;
}

VALUE_TYPE d_get_subfloat_default(struct cdict** d, char* subdict_name
                                 , char* name, VALUE_TYPE default_fval) {
    struct cdict *sub_dict, *element;
    sub_dict = _check_type_and_create(d, subdict_name, DICT_D, VERBOSE_LOGGING);

    element = NULL;
    HASH_FIND_STR(sub_dict->d_val, name, element);
    if (element == NULL || element->type != DICT_F) {
        /*
         * element does either not exist or has the wrong type.
         * Create a new / Overwrite the existing dict element with the default value.
         */

        if (element != NULL && element->type == DICT_I && AUTOCAST_DEFAULT) {
            /* element is an int32_t lets convert it to float */
            default_fval = element->val;
        }

        element = _check_type_and_create(&(sub_dict->d_val), name, DICT_F, VERBOSE_LOGGING);
        element->fval = default_fval;
    }

    return element->fval;
}

struct cdict** d_add_dlist(struct cdict** d, char* name) {
    struct cdict *element;
    element = _check_type_and_create(d, name, DICT_DL, VERBOSE_LOGGING);
    if (check_and_extend_array(element)) return &(element->d_list[element->val - 1]);
    return NULL;
}

struct cdict* d_next(struct cdict* element) {
    if (element == NULL) return NULL;
    return element->hh.next;
}

void dump_dict_element_as_json_to_file(struct cdict* element, FILE *file, int32_t indent) {
    uint64_t i;

    fprintf(file, "%*s" "\"%s\": ", indent, " ", element->name);

    if (element->type == DICT_DL
        || element->type == DICT_FL
        || element->type == DICT_IL
        || element->type == DICT_SL) {
        fprintf(file, "[ \n");
    }

    switch (element->type) {
        case DICT_I:
            fprintf(file, "%" PRINTF_INT64_MODIFIER "u",  element->val);
            break;
        case DICT_F:
            fprintf(file, "%.18f",element->fval);
            break;
        case DICT_D:
        	_dump_dict_as_json_to_file(&(element->d_val), file, indent, 1);
            break;
        case DICT_DL:
            for (i = 0; i < element->val; i++) {
            	_dump_dict_as_json_to_file(&(element->d_list[i]), file, indent + 3, 0);
                if (i != element->val - 1) fprintf(file, ",");
                fprintf(file, "\n");
            }
            break;
        case DICT_FL:
            for (i = 0; i < element->val; i++) {
                fprintf(file,"%*s" "%.18f", indent + 3, " ",element->fval_list[i]);
                if (i != element->val - 1) fprintf(file, ",");
                fprintf(file, "\n");
            }
            break;
        case DICT_IL:
            for (i = 0; i < element->val; i++) {
                fprintf(file, "%*s" "%" PRINTF_INT64_MODIFIER "u", indent + 3, " ",element->val_list[i]);
                if (i != element->val - 1) fprintf(file, ",");
                fprintf(file, "\n");
            }
            break;
        case DICT_ST:
            fprintf(file, "\"%s\"",element->sval);
            break;
        case DICT_SL:
            for (i = 0; i < element->val; i++) {
                fprintf(file, "%*s" "\"%s\"", indent + 3, " ",element->sval_list[i]);
                if (i != element->val - 1) fprintf(file, ", ");
                fprintf(file, "\n");
            }
            break;

        default: break;
    }

    if (element->type == DICT_DL
        || element->type == DICT_FL
        || element->type == DICT_IL
        || element->type == DICT_SL) {
        fprintf(file, "%*s" "]", indent, " ");
    }
}

void _dump_dict_as_json_to_file(struct cdict** d, FILE *file, int32_t indent, int32_t is_named_dict) {
    struct cdict *element, *tmp;
    uint64_t no_elements, i;
    element = NULL;
    tmp = NULL;

    no_elements = HASH_COUNT(*d);
    i = 0;

    if (is_named_dict) {
    	fprintf(file, "{\n");
    } else {
    	fprintf(file, "%*s" "{\n", indent, " ");
    }
    HASH_ITER(hh, *d, element, tmp) {
        dump_dict_element_as_json_to_file(element, file, indent + 3);
        i += 1;
        if (i != no_elements) fprintf(file, ",");
        fprintf(file, "\n");
    }
    fprintf(file, "%*s" "}", indent, " ");
}

void dump_dict_as_json_to_file(struct cdict** d, FILE *file) {
	_dump_dict_as_json_to_file(d, file, 0, 0);
}

#ifdef TEST_MODE
#include <stdio.h>

void add_testdata_to_cdict(struct cdict** d) {
    char buffer[100];
    uint64_t i;
    struct cdict** subdict;

    d_add_int(d, "test_int", 0);
    d_add_float(d, "test_float", 0.5);
    d_add_str(d, "test_str", "some test string");
    d_add_str(d, "test_str2", "some test string2");

    for (i = 0; i < 3 * ALLOC_CHUNKSIZE; i++) {
        d_add_ilist(d, "test_int_list", i);
        d_add_flist(d, "test_float_list", i);
        sprintf(buffer, "%" PRINTF_INT64_MODIFIER "u", i);
        d_add_slist(d, "test_string_list", buffer);
        subdict = d_add_dlist(d, "test_subdict_list");
        d_add_int(subdict, "test_int", i);
        d_add_str(subdict, "subdict_test_str2", "another test string2");
        d_add_subint(subdict, "subdict_name", "subdict_int_value_name", 3 * i);
        d_add_subfloat(subdict, "subdict_name", "subdict_float_value_name", 5.0 * i);
    }

    d_add_subint(d, "subdict_name", "subdict_int_value_name", 3);
    d_add_subfloat(d, "subdict_name", "subdict_float_value_name", 5.0);


    d_get_subint_default(d, "subdict_name", "subdict_int_value_name", 6);
    d_get_subfloat_default(d, "subdict_name", "subdict_float_value_name", 0.66);

    d_get_subfloat_default(d, "subdict_name", "set_by_default", 0.66);
    d_get_subint_default(d, "subdict_name", "subdict_float_value_name", 5);
    d_add_substring(d, "subdict_name", "subdict_string_value_name", "some string");
}

void overwrite_testdata_in_cdict(struct cdict** d) {
    char buffer[100];
    uint64_t i;
    struct cdict** subdict;

    d_add_int(d, "test_str", 0);
    d_add_float(d, "test_int", 0.5);
    d_add_str(d, "test_float", "added_string");
    d_add_str(d, "test_str2", "another test string2");

    for (i = 0; i < 3 * ALLOC_CHUNKSIZE; i++) {
        d_add_ilist(d, "test_string_list", i);
        d_add_flist(d, "test_int_list", i);
        sprintf(buffer, "%" PRINTF_INT64_MODIFIER "u", i);
        d_add_slist(d, "test_float_list", buffer);
        subdict = d_add_dlist(d, "test_float_list");
        d_add_int(subdict, "test_int", i);
        d_add_str(subdict, "subdict_test_str2", "another test string2");
    }

    d_add_subint(d, "subdict_name", "float_value_name", 4);
    d_add_subfloat(d, "subdict_name", "int_value_name", 2.0);
    d_add_substring(d, "subdict_name", "subdict_string_value_name", "some other string");

    d_get_subint_default(d, "subdict_name", "int_value_name", 3);
    d_get_subfloat_default(d, "subdict_name", "float_value_name", 1.5);
}

int main (int argc, char *argv[]) {
    struct cdict* main_cdict;
    struct cdict** sub_cdict;
    char buffer[100];
    uint64_t i;
    main_cdict = NULL;
    sub_cdict = NULL;

    add_testdata_to_cdict(&main_cdict);

#ifdef OUTPUT_FILE
    {
        FILE* f;

        f = fopen("testfile", "wb");
        if (!f) {
            printf("unable to open testfile for writing\n");
            return -1;
        }
        dump_dict_as_json_to_file(&main_cdict, f);
        fclose(f);
    }
#endif

    d_add_subfloat(&main_cdict, "subdict_name", "subdict_float_value_name_err", 5.5);
    d_get_subint_default(&main_cdict, "subdict_name", "subdict_float_value_name_err", 6);
    d_add_subfloat(&main_cdict, "subdict_name", "subdict_float_value_name_err", -1);
    d_get_subint_default(&main_cdict, "subdict_name", "subdict_float_value_name_err", 1);

    overwrite_testdata_in_cdict(&main_cdict);

    sub_cdict = d_add_cdict(&main_cdict, "internal_cdict");

    add_testdata_to_cdict(sub_cdict);
    overwrite_testdata_in_cdict(sub_cdict);
    d_add_str(sub_cdict, "test_str", "subcdict_string");
    for (i = 0; i < 4 * ALLOC_CHUNKSIZE; i++) {
        sprintf(buffer, "internal_cdict_list %" PRINTF_INT64_MODIFIER "u", i);
        sub_cdict = d_add_dlist(&main_cdict, buffer);
        if (i % 4 == 0) add_testdata_to_cdict(sub_cdict);
        if (i % 2 == 0) overwrite_testdata_in_cdict(sub_cdict);
    }

    free_cdict(&main_cdict);
    return 0;
}
#endif
