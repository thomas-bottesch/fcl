from libc.stdint cimport uint32_t
from cpython.exc cimport PyErr_CheckSignals

cdef public void check_signals(uint32_t* stop) with gil:
  PyErr_CheckSignals()