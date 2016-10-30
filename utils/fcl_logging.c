#include "fcl_logging.h"

#define LOG_TO(log_function)                                                     \
do {                                                                             \
   char buffer[10001];                                                           \
   va_list ap;                                                                   \
   va_start(ap, format);                                                         \
   vsnprintf (buffer, 10000, format, ap);                                        \
   va_end(ap);                                                                   \
   log_function(filename, functionname, line_no, buffer);                        \
} while (0)


extern void log_debug(char* filename, const char* functionname, uint32_t line_no, char* str);
extern void log_info(char* filename, const char* functionname, uint32_t line_no, char* str);
extern void log_error(char* filename, const char* functionname, uint32_t line_no, char* str);
extern void log_warning(char* filename, const char* functionname, uint32_t line_no, char* str);
extern void log_critical(char* filename, const char* functionname, uint32_t line_no, char* str);

void _log_debug(char* filename, const char* functionname, uint32_t line_no, const char *format, ...) {
    LOG_TO(log_debug);
}

void _log_info(char* filename, const char* functionname, uint32_t line_no, const char *format, ...) {
    LOG_TO(log_info);
}

void _log_critical(char* filename, const char* functionname, uint32_t line_no, const char *format, ...) {
    LOG_TO(log_critical);
}

void _log_warning(char* filename, const char* functionname, uint32_t line_no, const char *format, ...) {
    LOG_TO(log_warning);
}

void _log_error(char* filename, const char* functionname, uint32_t line_no, const char *format, ...) {
    LOG_TO(log_error);
}

#ifdef TEST_MODE
#include <stdio.h>

int main (int argc, char *argv[]) {
    LOG_DEBUG("hi");
    LOG_INFO("hi");
    LOG_ERROR("hi");
}
#endif
