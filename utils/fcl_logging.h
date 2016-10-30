#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "types.h"

#define _FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__

#define LOG_DEBUG(...) _log_debug(_FILE, __FUNCTION__, __LINE__,  FIRST(__VA_ARGS__) REST(__VA_ARGS__))
#define LOG_INFO(...) _log_info(_FILE, __FUNCTION__, __LINE__,  FIRST(__VA_ARGS__) REST(__VA_ARGS__))
#define LOG_ERROR(...) _log_error(_FILE, __FUNCTION__, __LINE__,  FIRST(__VA_ARGS__) REST(__VA_ARGS__))
#define LOG_WARNING(...) _log_warning(_FILE, __FUNCTION__, __LINE__,  FIRST(__VA_ARGS__) REST(__VA_ARGS__))
#define LOG_CRITICAL(...) _log_critical(_FILE, __FUNCTION__, __LINE__,  FIRST(__VA_ARGS__) REST(__VA_ARGS__))

/* expands to the first argument */
#define FIRST(...) FIRST_HELPER(__VA_ARGS__, throwaway)
#define FIRST_HELPER(first, ...) first

#define REST(...) REST_HELPER(NUM(__VA_ARGS__), __VA_ARGS__)
#define REST_HELPER(qty, ...) REST_HELPER2(qty, __VA_ARGS__)
#define REST_HELPER2(qty, ...) REST_HELPER_##qty(__VA_ARGS__)
#define REST_HELPER_ONE(first)
#define REST_HELPER_TWOORMORE(first, ...) , __VA_ARGS__
#define NUM(...) \
    SELECT_20TH(__VA_ARGS__, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE,\
                TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE,\
                TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE,\
                TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, ONE, throwaway)
#define SELECT_20TH(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,\
                    a16, a17, a18, a19, a20, ...) a20

void _log_debug(char* filename, const char* functionname, uint32_t line_no, const char *format, ...);
void _log_info(char* filename, const char* functionname, uint32_t line_no, const char *format, ...);
void _log_error(char* filename, const char* functionname, uint32_t line_no, const char *format, ...);
void _log_warning(char* filename, const char* functionname, uint32_t line_no, const char *format, ...);
void _log_critical(char* filename, const char* functionname, uint32_t line_no, const char *format, ...);
