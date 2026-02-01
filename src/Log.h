#ifndef LOG_H
#define LOG_H

#include <cstdio>
#include <cstdarg>

// Global logging flag
extern bool gLoggingEnabled;

// Initialize logging (call from main)
void InitLogging(bool enabled);

// Log functions
void Log(const char* format, ...);
void LogError(const char* format, ...);
void LogDebug(const char* format, ...);

// Macros for convenient logging
#define LOG(fmt, ...) Log(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LogError(fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LogDebug(fmt, ##__VA_ARGS__)

#endif // LOG_H
