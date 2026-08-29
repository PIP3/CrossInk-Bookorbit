#include "FileLogger.h"

#include <HalClock.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

FsFile FileLogger::logFile;
bool FileLogger::enabled = false;

void FileLogger::rotateLogsIfNeeded() {
    if (Storage.exists(LOG_FILE_PATH)) {
        FsFile file;
        if (Storage.openFileForRead("FILELOG", LOG_FILE_PATH, file)) {
            if (file.fileSize() > MAX_LOG_SIZE) {
                file.close();
                // Rename current log file
                Storage.rename(LOG_FILE_PATH, "/.crosspoint/debug_old.log");
                // Remove old log file if it exists
                Storage.remove("/.crosspoint/debug_old.log");
            }
            file.close();
        }
    }
}

void FileLogger::log(const char* tag, const char* message) {
    if (!enabled) return;
    
    if (!logFile) {
        // Create directory if it doesn't exist
        if (!Storage.exists("/.crosspoint")) {
            Storage.mkdir("/.crosspoint");
        }
        
        // Rotate logs if needed
        rotateLogsIfNeeded();
        
        // Open log file in append mode
        if (!Storage.openFileForWrite("FILELOG", LOG_FILE_PATH, logFile)) {
            return;
        }
    }
    
    if (logFile) {
        // Get timestamp if available
        char timestamp[20] = "";
        uint8_t utcHour = 0, utcMinute = 0, utcSecond = 0;
        uint16_t utcYear = 0;
        uint8_t utcMonth = 0, utcDay = 0;
        if (halClock.getDateTime(utcYear, utcMonth, utcDay, utcHour, utcMinute)) {
            snprintf(timestamp, sizeof(timestamp), "%04u-%02u-%02u %02u:%02u:00", 
                    utcYear, utcMonth, utcDay, utcHour, utcMinute);
        }
        
        char buffer[256];
        if (timestamp[0] != '\0') {
            snprintf(buffer, sizeof(buffer), "[%s] %s: %s\n", timestamp, tag, message);
        } else {
            snprintf(buffer, sizeof(buffer), "%s: %s\n", tag, message);
        }
        
        size_t len = strlen(buffer);
        if (logFile.write(reinterpret_cast<const uint8_t*>(buffer), len) != len) {
            // Write failed, close the file to avoid corruption
            logFile.close();
            logFile = FsFile();
        }
    }
}

void FileLogger::log(const char* tag, const std::string& message) {
    log(tag, message.c_str());
}

void FileLogger::logf(const char* tag, const char* format, ...) {
    if (!enabled) return;
    
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log(tag, buffer);
}

void FileLogger::flush() {
    if (logFile) {
        logFile.flush();
    }
}

void FileLogger::close() {
    if (logFile) {
        logFile.flush();
        logFile.close();
        logFile = FsFile();
    }
}

bool FileLogger::isEnabled() {
    return enabled;
}

void FileLogger::setEnabled(bool enable) {
    enabled = enable;
    if (!enable) {
        close();
    }
}
