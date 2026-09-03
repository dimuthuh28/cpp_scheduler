#pragma once

#include <string>

// Minimal thread-safe logger: serializes writes to stdout/stderr with a mutex
// so concurrent worker threads don't interleave partial lines.
class Logger {
public:
    static void info(const std::string& message);
    static void error(const std::string& message);

private:
    static void log(const char* level, const std::string& message);
};
