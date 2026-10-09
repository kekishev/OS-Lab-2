#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/common.h"

static int is_vowel(char c) {
    switch (c) {
        case 'a': case 'e': case 'i': case 'o': case 'u':
        case 'A': case 'E': case 'I': case 'O': case 'U':
            return 1;
        default:
            return 0;
    }
}

static void process_line(HANDLE out, const char* line, size_t len) {
    char* result = (char *) malloc(len + 1);
    if (!result) {
        fail_mem("child");
    }

    size_t k = 0;
    for (size_t i = 0; i < len; i++)
        if (!is_vowel(line[i])) {
            result[k++] = line[i];
        }

    result[k++] = '\n';

    write_all(out, result, k, "WriteFile: unsuccessful write to file operation", "child");
    free(result);
    result = NULL;
}

int main(int argc, char **argv) {
    char* level_name = "child";

    if (argc > 1) {
        level_name = argv[1];
    }

    HANDLE in  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    if (in == INVALID_HANDLE_VALUE) {
        fail("GetStdHandle (standard input)", level_name);
    }
    if (out == INVALID_HANDLE_VALUE) {
        fail("GetStdHandle (standard output)", level_name);
    }

    setvbuf(stderr, NULL, _IOFBF, 4096);

    fprintf(stderr, "[%s] running, PID=%lu\n", level_name,(unsigned long) GetCurrentProcessId());
    fflush(stderr);

    size_t cap = 256;
    char* accumulator = (char *) malloc(cap);

    if (!accumulator) {
        fail_mem(level_name);
        return 1;
    }

    size_t len = 0;
    char buf[4096];
    DWORD got;
    while (1) {
        if (!ReadFile(in, buf, (DWORD)sizeof(buf), &got, NULL)) {
            DWORD code = GetLastError();
            if (code == ERROR_BROKEN_PIPE) {
                 break;
            }

            SetLastError(code);
            fail("ReadFile (Reading from the pipe)", level_name);
        }

        if (got == 0) {
            break;
        }

        for (DWORD i = 0; i < got; i++) {
            char c = buf[i];

            if (c == '\r') {
                continue;
            }
            if (c == '\n') {
                process_line(out, accumulator, len);
                len = 0;
                continue;
            }

            if (len + 1 >= cap) {
                cap *= 2;
                char* bigger = (char *) realloc(accumulator, cap);

                if (!bigger) {
                    free(accumulator);
                    fail_mem(level_name);
                }

                accumulator = bigger;
            }

            accumulator[len++] = c;
        }
    }

    if (len > 0) {
        process_line(out, accumulator, len);
    }

    free(accumulator);
    accumulator = NULL;

    fprintf(stderr, "[%s] finished, PID=%lu\n", level_name, (unsigned long) GetCurrentProcessId());
    fflush(stderr);

    return 0;
}
