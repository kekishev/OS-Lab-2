#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common/common.h"

#define CHILD_EXE "child.exe"
#define PIPE1_CHANCE 80

static char* read_line_from_stdin(void) {
    size_t cap = 64;
    size_t len = 0;

    char* buf = (char *) malloc(cap);
    int c;

    if (!buf) {
        fail_mem("parent");
        buf = NULL;
        return NULL;
    }

    while ((c = fgetc(stdin)) != EOF) {
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            buf[len] = '\0';
            return buf;
        }

        if (len + 1 >= cap) {
            char* bigger;
            cap *= 2;
            bigger = (char *) realloc(buf, cap);

            if (!bigger) {
                free(buf);
                fail_mem("parent");
                return NULL;
            }

            buf = bigger;
        }

        buf[len++] = (char) c;
    }

    if (len == 0) {
        free(buf);
        buf = NULL;
        return NULL;
    }

    buf[len] = '\0';
    return buf;
}

static void set_inherit_descriptor(HANDLE handle, BOOL is_enable) {
    if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT,is_enable ? HANDLE_FLAG_INHERIT : 0)) {
        fail("SetHandleInformation", "parent");
    }
}

static HANDLE launch_child(const char* label, HANDLE handle_in, HANDLE handle_out, DWORD* pid) {
    STARTUPINFOA si; // The description of how a new process should start
    PROCESS_INFORMATION pi;
    char cmdline[512];

    set_inherit_descriptor(handle_in,  TRUE);
    set_inherit_descriptor(handle_out, TRUE);

    /**
     * ZeroMemory makes every field be NULL except defined ones.
     * So, Windows applies default settings for them.
     */
    ZeroMemory(&si, sizeof(si));

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = handle_in;
    si.hStdOutput = handle_out;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    ZeroMemory(&pi, sizeof(pi));

    // snprintf fills string with the message
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", CHILD_EXE, label);

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        fail("CreateProcess (launch " CHILD_EXE ")", "parent");
    }

    set_inherit_descriptor(handle_in,  FALSE);
    set_inherit_descriptor(handle_out, FALSE);

    CloseHandle(pi.hThread);
    *pid = pi.dwProcessId;

    return pi.hProcess;
}

int main(void) {
    HANDLE pipe1_r, pipe1_w, pipe2_r, pipe2_w, file1, file2, child1, child2;

    DWORD pid1 = 0, pid2 = 0, code1 = 0, code2 = 0;
    char* name1, *name2, *line;
    unsigned long sent1 = 0, sent2 = 0;

    setvbuf(stderr, NULL, _IOFBF, 4096); // Buffer for cout

    fprintf(stderr, "[parent] is running, PID=%lu\n", (unsigned long) GetCurrentProcessId());

    fprintf(stderr, "Enter a name of file for child1: ");
    fflush(stderr); // sends everything to the screen

    name1 = read_line_from_stdin();
    if (!name1 || name1[0] == '\0') {
        fprintf(stderr, "[parent] ERROR: filename for child1 is not defined\n");
        fflush(stderr);
        return 1;
    }

    fprintf(stderr, "Enter a name of file for child2: ");
    fflush(stderr);

    name2 = read_line_from_stdin();
    if (!name2 || name2[0] == '\0') {
        fprintf(stderr, "[parent] ERROR: filename for child2 is not defined\n");
        fflush(stderr);
        return 1;
    }

    if (!CreatePipe(&pipe1_r, &pipe1_w, NULL, 0)) {
        fail("CreatePipe (pipe1)", "parent");
    }
    if (!CreatePipe(&pipe2_r, &pipe2_w, NULL, 0)) {
        fail("CreatePipe (pipe2)", "parent");
    }

    file1 = CreateFileA(name1, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file1 == INVALID_HANDLE_VALUE) {
        fail("CreateFile (file for child1)", "parent");
        return 1;
    }

    file2 = CreateFileA(name2, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file2 == INVALID_HANDLE_VALUE) {
        fail("CreateFile (file for child2)", "parent");
        return 1;
    }

    child1 = launch_child("child1", pipe1_r, file1, &pid1);
    child2 = launch_child("child2", pipe2_r, file2, &pid2);

    fprintf(stderr, "[parent] child1 PID=%lu -> %s\n", (unsigned long) pid1, name1);
    fprintf(stderr, "[parent] child2 PID=%lu -> %s\n", (unsigned long) pid2, name2);
    fflush(stderr);

    CloseHandle(pipe1_r);
    CloseHandle(pipe2_r);
    CloseHandle(file1);
    CloseHandle(file2);

    srand((unsigned)time(NULL) ^ GetCurrentProcessId());

    fprintf(stderr, "[parent] Enter lines; The end - Ctrl+Z and Enter\n");
    fflush(stderr);

    while ((line = read_line_from_stdin()) != NULL) {
        int to_first = (rand() % 100) < PIPE1_CHANCE;
        HANDLE target = to_first ? pipe1_w : pipe2_w;
        size_t len = strlen(line);

        write_all(target, line, len, to_first ? "WriteFile (pipe1)" : "WriteFile (pipe2)", "parent");
        write_all(target, "\n", 1, to_first ? "WriteFile (pipe1)" : "WriteFile (pipe2)", "parent");

        if (to_first) {
            sent1++;
        } else {
            sent2++;
        }

        fprintf(stderr, "[parent] line (%u symbols) -> %s\n", (unsigned)len, to_first ? "pipe1" : "pipe2");
        fflush(stderr);

        free(line);
    }

    CloseHandle(pipe1_w);
    CloseHandle(pipe2_w);

    if (WaitForSingleObject(child1, INFINITE) == WAIT_FAILED) {
        fail("WaitForSingleObject (child1)", "parent");
    }
    if (WaitForSingleObject(child2, INFINITE) == WAIT_FAILED) {
        fail("WaitForSingleObject (child2)", "parent");
    }

    if (!GetExitCodeProcess(child1, &code1)) {
        fail("GetExitCodeProcess (child1)", "parent");
    }
    if (!GetExitCodeProcess(child2, &code2)) {
        fail("GetExitCodeProcess (child2)", "parent");
    }

    fprintf(stderr, "[parent] sent: pipe1 = %lu, pipe2 = %lu (Overall %lu)\n", sent1, sent2, sent1 + sent2);
    fprintf(stderr, "[parent] child1 (PID=%lu) finished with code %lu\n", (unsigned long) pid1, (unsigned long) code1);
    fprintf(stderr, "[parent] child2 (PID=%lu) finished with code %lu\n", (unsigned long) pid2, (unsigned long) code2);

    CloseHandle(child1);
    CloseHandle(child2);
    free(name1);
    free(name2);
    name1 = NULL;
    name2 = NULL;

    return 0;
}
