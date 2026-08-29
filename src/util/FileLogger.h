#pragma once

#include <HalStorage.h>
#include <string>

/**
 * Simple file logger for debugging purposes.
 * Writes logs to a file on the SD card for later analysis.
 */
class FileLogger {
public:
    static void log(const char* tag, const char* message);
    static void log(const char* tag, const std::string& message);
    static void logf(const char* tag, const char* format, ...);
    
    static void flush();
    static void close();
    
    static bool isEnabled();
    static void setEnabled(bool enabled);
    
private:
    static constexpr const char* LOG_FILE_PATH = "/.crosspoint/debug.log";
    static constexpr size_t MAX_LOG_SIZE = 1024 * 1024; // 1MB
    
    static FsFile logFile;
    static bool enabled;
    
    static void rotateLogsIfNeeded();
};
