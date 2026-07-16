using System;
using System.IO;

namespace TTPFrontEnd.Utils;

/// <summary>
/// 简易文件日志。日志写入 %LocalAppData%/TTPFrontEnd/log/log_yyyyMMdd_HHmmss.txt，
/// 每次启动一个文件，自动带时间戳。
/// </summary>
public static class Logger
{
    private static readonly object _lock = new();

    private static string? _logPath;
    private static string LogPath => _logPath ??= InitLogPath();

    private static string InitLogPath()
    {
        var now = DateTime.Now;
        var name = $"log_{now:yyyyMMdd}_{now:HHmmss}.txt";
        return Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "TTPFrontEnd", "log", name);
    }

    public static void Info(string message)   => Write("INFO", message);
    public static void Warn(string message)   => Write("WARN", message);
    public static void Error(string message)  => Write("ERROR", message);
    public static void Request(string method, string url)
        => Write("REQ", $"{method} {url}");

    public static void Response(int statusCode, string url)
        => Write("RES", $"{statusCode} {url}");

    private static void Write(string level, string message)
    {
        var line = $"{DateTime.Now:yyyy-MM-dd HH:mm:ss.fff} [{level}] {message}";
        lock (_lock)
        {
            try
            {
                var path = LogPath;
                var dir = Path.GetDirectoryName(path);
                if (!Directory.Exists(dir))
                    Directory.CreateDirectory(dir!);
                File.AppendAllText(path, line + Environment.NewLine);
            }
            catch { /* 日志写入失败不影响主流程 */ }
        }
    }
}
