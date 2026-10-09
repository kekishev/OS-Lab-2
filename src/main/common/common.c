#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

void fail(const char *error, const char* level_name) {
    DWORD code = GetLastError();
    char *text = NULL;

    FormatMessageA(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Language of the message
        (LPSTR)&text, // Saving results
        0,
        NULL
        );

    fprintf(stderr, "[%s] ERROR: %s. Code %lu: %s\n", level_name, error, (unsigned long) code, text ? text : "(Description is unreachable)");
    fflush(stderr);

    if (text) {
        LocalFree(text);
    }

    ExitProcess(1);
}

void fail_mem(const char* process_name) {
    fprintf(stderr, "[%s] ERROR: Memory allocation is not succeeded\n", process_name);
    fflush(stderr);
    ExitProcess(1);
}

void write_all(HANDLE place_to_write, const void* data, size_t size, const char* error_message, const char* level_name) {
    const char* text_to_write = (const char *) data;

    while (size > 0) {
        DWORD chunk = (size > 0x10000u) ? 0x10000u : (DWORD)size;
        DWORD number_of_written_bytes = 0;

        if (!WriteFile(place_to_write, text_to_write, chunk, &number_of_written_bytes, NULL)) {
            fail(error_message, level_name);
            break;
        }
        if (number_of_written_bytes == 0) {
            fail(error_message, level_name);
            break;
        }

        text_to_write += number_of_written_bytes;
        size -= number_of_written_bytes;
    }
}
