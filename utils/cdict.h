#ifndef CDICT_H
#define CDICT_H

#include "uthash.h"
#include "types.h"
#include "stdio.h"

#define DICT_I  UINT32_C(0)     /**< cdict data type single integer */
#define DICT_F  UINT32_C(1)     /**< cdict data type single float */
#define DICT_D  UINT32_C(2)     /**< cdict data type cdict */
#define DICT_DL UINT32_C(3)     /**< cdict data type list of cdict */
#define DICT_FL UINT32_C(4)     /**< cdict data type list of floats */
#define DICT_IL UINT32_C(5)     /**< cdict data type list of unsigned integers */
#define DICT_ST UINT32_C(6)     /**< cdict data type is string */
#define DICT_SL UINT32_C(7)     /**< cdict data type is list of strings */

struct cdict {
    char* name;                     /**< The name of this cdict element. */
    uint32_t type;                  /**< The type of the data stored in this element */
    VALUE_TYPE fval;                /**< The floating point value of this data element */
    uint64_t val;                   /**< Unsigned integer value. / filling level of lists. */
    uint64_t alloced;               /**< Unsigned integer value tells how many list elements were allocated. */
    uint64_t* val_list;             /**< List of integer values. */
    VALUE_TYPE* fval_list;          /**< List of floating point values. */
    char** sval_list;               /**< List of string values. */
    char* sval;                     /**< The string value of this element */
    struct cdict* d_val;            /**< Hashmap value of this element*/
    struct cdict** d_list;           /**< List of cdicts */
    UT_hash_handle hh;              /**< ut_hash element which makes this structure hashable */
};

void free_cdict(struct cdict** d);
void delete_cdict_element(struct cdict** d, char* name);
void d_add_int(struct cdict** d, char* name, uint64_t val);
void d_add_str(struct cdict** d, char* name, char* str);
void d_add_float(struct cdict** d, char* name, VALUE_TYPE val);
void d_add_ilist(struct cdict** d, char* name, uint64_t val);
void d_add_flist(struct cdict** d, char* name, VALUE_TYPE fval);
void d_add_slist(struct cdict** d, char* name, char* str);
void d_add_subint(struct cdict** d, char* subdict_name, char* name, uint64_t val);
void d_add_subfloat(struct cdict** d, char* subdict_name, char* name, VALUE_TYPE fval);
void d_add_substring(struct cdict** d, char* subdict_name, char* name, char* sval);

uint64_t d_get_subint_default(struct cdict** d, char* subdict_name
                             , char* name, uint64_t default_val);
VALUE_TYPE d_get_subfloat_default(struct cdict** d, char* subdict_name
                                 , char* name, VALUE_TYPE default_fval);
struct cdict** d_add_cdict(struct cdict** d, char* name);
struct cdict** d_add_dlist(struct cdict** d, char* name);
struct cdict* d_next(struct cdict* element);
void dump_dict_as_json_to_file(struct cdict** d, FILE *file);

#endif /* CDICT_H */
