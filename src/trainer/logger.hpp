#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <string>
#include <mutex>

static FILE* g_logFile = nullptr;
static std::mutex g_logMutex;

static void LogInit(const char* dir, const char* filename = "paltrainer_log.txt") {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile) return;
    char path[MAX_PATH];
    if (dir && dir[0])
        snprintf(path, MAX_PATH, "%s\\%s", dir, filename);
    else
        strcpy_s(path, filename);
    g_logFile = fopen(path, "w");
    if (g_logFile) {
        time_t now = time(nullptr);
        char ts[64];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(g_logFile, "=== PalTrainerUltra Log ===\n[%s] Log started\n", ts);
        fflush(g_logFile);
    }
}

static void Log(const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_logFile) return;
    time_t now = time(nullptr);
    char ts[32];
    strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&now));
    fprintf(g_logFile, "[%s] ", ts);
    va_list args;
    va_start(args, fmt);
    vfprintf(g_logFile, fmt, args);
    va_end(args);
    fprintf(g_logFile, "\n");
    fflush(g_logFile);
}

static void LogClose() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile) {
        Log("Log closing.");
        fclose(g_logFile);
        g_logFile = nullptr;
    }
}
