cdef extern from "<utils/global_defs.h>":    
    void omp_set_num_threads(int num_threads);