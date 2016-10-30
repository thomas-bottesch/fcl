#define C_LOGGING_DO_NOT_LOG_POSITION 0
#define C_LOGGING_LOG_POSITION 1

#include <time.h>
#include "types.h"
#include <stdio.h>

#if defined(MATLAB_EXTENSION) || defined(OCTAVE_EXTENSION)
#include <mex.h>
#endif

void get_timestamp_as_string(char* str_date, size_t sizeof_str_date) {
    struct tm *tm;
    time_t t;

    t = time(NULL);
    tm = localtime(&t);

    strftime(str_date, sizeof_str_date, "%Y-%m-%d %H:%M:%S", tm);
}

void log_general(const char* tag, char* filename, const char* functionname, uint32_t line_no, char* str, uint32_t log_postition) {
    char str_date[100];
    char file_line[200];
    get_timestamp_as_string(str_date, sizeof(str_date));

    if (!log_postition) {
        file_line[0] = '\0';
    } else {
        snprintf (file_line, sizeof(file_line), " %s/%s(%d)", filename, functionname, line_no);
    }

#if defined(MATLAB_EXTENSION) || defined(OCTAVE_EXTENSION)
    mexPrintf("%s %s%s %s\n", str_date, tag, file_line, str);
    mexEvalString("pause(.001);");
#else
    printf("%s %s%s %s\n", str_date, tag, file_line, str);
    fflush(stdout);
#endif
}

void log_debug(char* filename, const char* functionname, uint32_t line_no, char* str) {
    log_general("[DEBUG  ]", filename, functionname, line_no, str, C_LOGGING_DO_NOT_LOG_POSITION);
}

void log_info(char* filename, const char* functionname, uint32_t line_no, char* str) {
    log_general("[INFO   ]", filename, functionname, line_no, str, C_LOGGING_DO_NOT_LOG_POSITION);
}

void log_error(char* filename, const char* functionname, uint32_t line_no, char* str) {
    log_general("[ERROR  ]", filename, functionname, line_no, str, C_LOGGING_LOG_POSITION);
}

void log_warning(char* filename, const char* functionname, uint32_t line_no, char* str) {
    log_general("[WARNING]", filename, functionname, line_no, str, C_LOGGING_LOG_POSITION);
}

void log_critical(char* filename, const char* functionname, uint32_t line_no, char* str) {
    log_general("[CRITICAL]", filename, functionname, line_no, str, C_LOGGING_LOG_POSITION);
}
