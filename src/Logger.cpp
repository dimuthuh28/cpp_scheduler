#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

namespace {
std::mutex g_logMutex;

std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}
}  // namespace

void Logger::log(const char* level, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::cout << "[" << timestamp() << "] [" << level << "] [thread "
              << std::this_thread::get_id() << "] " << message << std::endl;
}

void Logger::info(const std::string& message) { log("INFO", message); }

void Logger::error(const std::string& message) { log("ERROR", message); }
