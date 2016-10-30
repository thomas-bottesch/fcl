#include "kmeans_cluster_hashmap.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <ctype.h>
#include "../../utils/fcl_logging.h"

uint32_t add_sample_to_hashmap(struct keyvaluecount_hash** clusters_raw
                                      , KEY_TYPE* keys
                                      , VALUE_TYPE* values
                                      , uint64_t nnz
                                      , uint64_t cluster_id) {
    uint64_t sample_iter;
    struct keyvaluecount_hash* cluster_entry;
    uint32_t item_added;

    item_added = 0;
    for (sample_iter = 0; sample_iter  < nnz; sample_iter++) {
        cluster_entry = NULL;
        HASH_FIND_INT(clusters_raw[cluster_id], keys + sample_iter, cluster_entry);
        if (cluster_entry==NULL) {
            cluster_entry = (struct keyvaluecount_hash*) malloc(sizeof(struct keyvaluecount_hash));
            cluster_entry->id = *(keys + sample_iter);
            cluster_entry->val = *(values + sample_iter);
            cluster_entry->count = 1;
            item_added = 1;
            HASH_ADD_INT(clusters_raw[cluster_id], id, cluster_entry);
        } else {
            cluster_entry->val += *(values + sample_iter);
            cluster_entry->count += 1;
        }
    }
    return item_added;
}

uint32_t add_sample_to_hashmap_minibatch_kmeans(struct keyvaluecount_hash** clusters_raw
                                      , KEY_TYPE* keys
                                      , VALUE_TYPE* values
                                      , uint64_t nnz
                                      , uint64_t cluster_id
                                      , uint64_t cluster_count) {
    uint64_t sample_iter;
    struct keyvaluecount_hash* cluster_entry;
    uint32_t item_added;

    struct keyvaluecount_hash *tmp;

    HASH_ITER(hh, clusters_raw[cluster_id], cluster_entry, tmp) {
        cluster_entry->val -= cluster_entry->val / (cluster_count + 1);
    }

    item_added = 0;
    for (sample_iter = 0; sample_iter  < nnz; sample_iter++) {
        cluster_entry = NULL;
        HASH_FIND_INT(clusters_raw[cluster_id], keys + sample_iter, cluster_entry);
        if (cluster_entry==NULL) {
            cluster_entry = (struct keyvaluecount_hash*) malloc(sizeof(struct keyvaluecount_hash));
            cluster_entry->id = *(keys + sample_iter);
            cluster_entry->val = (*(values + sample_iter) / (cluster_count + 1));
            item_added = 1;
            HASH_ADD_INT(clusters_raw[cluster_id], id, cluster_entry);
        } else {
            cluster_entry->val += (*(values + sample_iter) / (cluster_count + 1));
        }
    }
    return item_added;
}

void remove_sample_from_hashmap(struct keyvaluecount_hash** clusters_raw
                                       , KEY_TYPE* keys
                                       , VALUE_TYPE* values
                                       , uint64_t nnz
                                       , uint64_t cluster_id) {
    uint64_t sample_iter;
    struct keyvaluecount_hash* cluster_entry;

    for (sample_iter = 0; sample_iter  < nnz; sample_iter++) {
        cluster_entry = NULL;
        HASH_FIND_INT(clusters_raw[cluster_id], keys + sample_iter, cluster_entry);
        if (cluster_entry==NULL) {
            LOG_ERROR("expected element in hashmap but it is not available!");
        } else {
            cluster_entry->val -= *(values + sample_iter);
            cluster_entry->count -= 1;

            if (cluster_entry->count == 0) {
                HASH_DEL(clusters_raw[cluster_id], cluster_entry);
                free(cluster_entry);
            }
        }
    }
}

int id_sort(struct keyvaluecount_hash *a, struct keyvaluecount_hash *b) {
    if (a->id < b->id) return -1;
    if (a->id > b->id) return 1;
    return 0;

}

void free_keyvaluecount_hashmap(struct keyvaluecount_hash *k) {
    struct keyvaluecount_hash *current_item, *tmp;

    HASH_ITER(hh, k, current_item, tmp) {
      HASH_DEL(k,current_item);  /* delete; users advances to next */
      free(current_item);            /* optional- if you want to free  */
    }
}

void free_cluster_hashmaps(struct keyvaluecount_hash ** clusters
                           , uint64_t no_clusters) {
    uint64_t i;

  for (i = 0; i < no_clusters; i++) {
      free_keyvaluecount_hashmap(clusters[i]);
      clusters[i] = NULL;
  }
}

void create_matrix_from_hashmap(struct keyvaluecount_hash** clusters_raw
                                       , uint64_t* cluster_counts
                                       , struct csr_matrix *clusters) {
    uint64_t i, nnz;
    nnz = 0;

    /* calculate number of non zero values */
    for (i = 0; i < clusters->sample_count; i++) {
        nnz += HASH_COUNT(clusters_raw[i]);
        clusters->pointers[i + 1] = nnz;
    }

    /* allocate sparse cluster matrix */
    clusters->keys = (KEY_TYPE*) calloc(nnz, sizeof(KEY_TYPE));
    clusters->values = (VALUE_TYPE*) calloc(nnz, sizeof(VALUE_TYPE));

    /* fill sparse cluster matrix */
    for (i = 0; i < clusters->sample_count; i++) {
        uint64_t local_feature_count;
        struct keyvaluecount_hash *current_item, *tmp;
        current_item = NULL;
        tmp = NULL;

        local_feature_count = 0;
        HASH_ITER(hh, clusters_raw[i], current_item, tmp) {
          clusters->keys[clusters->pointers[i] + local_feature_count] = current_item->id;
          clusters->values[clusters->pointers[i] + local_feature_count] = current_item->val / cluster_counts[i];
          local_feature_count += 1;
        }
    }
}

void create_vector_list_from_hashmap(struct keyvaluecount_hash** clusters_raw
                                       , uint64_t* cluster_counts
                                       , struct sparse_vector *clusters
                                       , uint64_t no_cluster) {
    uint64_t i;

    /* fill sparse cluster matrix */
    for (i = 0; i < no_cluster; i++) {
        uint64_t local_feature_count;
        struct keyvaluecount_hash *current_item, *tmp;
        current_item = NULL;
        tmp = NULL;

        clusters[i].nnz = HASH_COUNT(clusters_raw[i]);
        if (clusters[i].nnz > 0) {
            clusters[i].keys = (KEY_TYPE*) calloc(clusters[i].nnz, sizeof(KEY_TYPE));
            clusters[i].values = (VALUE_TYPE*) calloc(clusters[i].nnz, sizeof(VALUE_TYPE));
        }

        local_feature_count = 0;
        HASH_ITER(hh, clusters_raw[i], current_item, tmp) {
          clusters[i].keys[local_feature_count] = current_item->id;
          clusters[i].values[local_feature_count] = current_item->val / cluster_counts[i];
          local_feature_count += 1;
        }
    }
}
