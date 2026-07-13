#pragma once

#include <filesystem>
#include <mutex>
#include <string>

class Logger
{
public:
    static void Initialize();
    static void Log(const std::string& message);
    static void Info(const std::string& message);
    static void Warn(const std::string& message);
    static void Error(const std::string& message);
    static std::filesystem::path LogFilePath();

private:
    static std::filesystem::path ResolveLogDirectory();
    static std::string Timestamp();
    static void LogWithLevel(const std::string& level, const std::string& message);

    static std::mutex mutex_;
    static std::filesystem::path logFilePath_;
};
