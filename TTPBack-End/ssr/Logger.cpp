#include "Logger.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

std::mutex Logger::mutex_;
std::filesystem::path Logger::logFilePath_;

namespace
{
std::string FileTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime {};
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y%m%d-%H%M%S");
    return stream.str();
}
}

void Logger::Initialize()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!logFilePath_.empty())
    {
        return;
    }

    const std::filesystem::path logDirectory = ResolveLogDirectory();
    std::filesystem::create_directories(logDirectory);
    logFilePath_ = logDirectory / ("run-" + FileTimestamp() + ".txt");

    std::ofstream output(logFilePath_, std::ios::app);
    output << "[" << Timestamp() << "] [INFO] logger initialized" << '\n';
}

void Logger::Log(const std::string& message)
{
    Info(message);
}

void Logger::Info(const std::string& message)
{
    LogWithLevel("INFO", message);
}

void Logger::Warn(const std::string& message)
{
    LogWithLevel("WARN", message);
}

void Logger::Error(const std::string& message)
{
    LogWithLevel("ERROR", message);
}

void Logger::LogWithLevel(const std::string& level, const std::string& message)
{
    Initialize();

    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream output(logFilePath_, std::ios::app);
    output << "[" << Timestamp() << "] [" << level << "] " << message << '\n';
}

std::filesystem::path Logger::LogFilePath()
{
    Initialize();

    std::lock_guard<std::mutex> lock(mutex_);
    return logFilePath_;
}

std::filesystem::path Logger::ResolveLogDirectory()
{
    if (std::filesystem::exists("Back-end"))
    {
        return std::filesystem::path("Back-end") / "logs";
    }

    if (std::filesystem::exists("TranTicketBack-end"))
    {
        return std::filesystem::path("TranTicketBack-end") / "logs";
    }

    return "logs";
}

std::string Logger::Timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime {};
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}
