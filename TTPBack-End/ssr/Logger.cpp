// 文件说明：日志模块：按时间和等级将运行信息写入日志文件。

#include "Logger.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

std::mutex Logger::mutex_;
std::filesystem::path Logger::logFilePath_;

namespace {
std::string FileTimestamp() {
    // 处理日志相关信息，便于查看程序运行情况。
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

void Logger::Initialize() {
    // 程序启动时创建日志目录，并确定本次运行使用的日志文件。
    std::lock_guard<std::mutex> lock(mutex_);
    // 先确认目标数据存在，避免访问无效内容。
    if (!logFilePath_.empty())
        return;
    const std::filesystem::path logDirectory = ResolveLogDirectory();
    std::filesystem::create_directories(logDirectory);
    logFilePath_ = logDirectory / ("run-" + FileTimestamp() + ".txt");

    std::ofstream output(logFilePath_, std::ios::app);
    output << "[" << Timestamp() << "] [INFO] logger initialized" << '\n';
}

void Logger::Log(const std::string& message) {
    // 处理日志相关信息，便于查看程序运行情况。
    Info(message);
}

void Logger::Info(const std::string& message) {
    // 执行该函数对应的处理，并返回结果。
    LogWithLevel("INFO", message);
}

void Logger::Warn(const std::string& message) {
    // 执行该函数对应的处理，并返回结果。
    LogWithLevel("WARN", message);
}

void Logger::Error(const std::string& message) {
    // 执行该函数对应的处理，并返回结果。
    LogWithLevel("ERROR", message);
}

void Logger::LogWithLevel(const std::string& level, const std::string& message) {
    // 所有日志最终都会汇总到这里，统一写入时间、等级和内容。
    Initialize();

    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream output(logFilePath_, std::ios::app);
    output << "[" << Timestamp() << "] [" << level << "] " << message << '\n';
}

std::filesystem::path Logger::LogFilePath() {
    // 处理日志相关信息，便于查看程序运行情况。
    Initialize();

    std::lock_guard<std::mutex> lock(mutex_);
    return logFilePath_;
}

std::filesystem::path Logger::ResolveLogDirectory() {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    if (std::filesystem::exists("Back-end"))
        return std::filesystem::path("Back-end") / "logs";
    // 根据当前条件决定是否进入下一步处理。
    if (std::filesystem::exists("TranTicketBack-end"))
        return std::filesystem::path("TranTicketBack-end") / "logs";
    return "logs";
}

std::string Logger::Timestamp() {
    // 处理日志相关信息，便于查看程序运行情况。
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
