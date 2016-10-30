#ifndef GLOBAL_DEFS_H
#define GLOBAL_DEFS_H

#ifdef _OPENMP
#include <omp.h>
#else
typedef int omp_lock_t;
#define omp_init_lock(x)
#define omp_destroy_lock(x)
#define omp_set_lock(x)
#define omp_unset_lock(x)
#define omp_set_dynamic(x)
#define omp_set_num_threads(x)
#define omp_get_max_threads() 1
#define omp_get_thread_num() 0
#endif

#ifndef EXTENSION
#define check_signals(X)
#else
extern void check_signals(uint32_t* stop);
#endif

#define xstr(s) str(s)
#define str(s) #s

#define free_null(ptr) if (ptr) {free(ptr); ptr = NULL;}

#endif /* GLOBAL_DEFS_H */
