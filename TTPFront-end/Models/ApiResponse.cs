namespace TTPFrontEnd.Models;

/// 后端统一响应包层
public class ApiResponse<T>
{
    public int Code { get; set; }
    public string Message { get; set; } = string.Empty;
    public T? Data { get; set; }
}
