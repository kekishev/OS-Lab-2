#pragma once

#include <windows.h>

void fail(const char *error, const char* level_name);

void fail_mem(const char* process_name);

void write_all(HANDLE place_to_write, const void* data, size_t size, const char* error_message, const char* level_name);
