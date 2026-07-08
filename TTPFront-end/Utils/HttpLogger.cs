using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;

namespace TTPFrontEnd.Utils;

/// <summary>
/// 带日志的 HttpClient 包装方法。
/// </summary>
public static class HttpLogger
{
    /// <summary>
    /// GET 请求 + 日志
    /// </summary>
    public static async Task<HttpResponseMessage> GetAsync(HttpClient client, string url)
    {
        Logger.Request("GET", url);
        var resp = await client.GetAsync(url);
        Logger.Response((int)resp.StatusCode, url);
        return resp;
    }

    /// <summary>
    /// POST + JSON + 日志
    /// </summary>
    public static async Task<HttpResponseMessage> PostAsJsonAsync<T>(HttpClient client, string url, T body)
    {
        Logger.Request("POST", url);
        var resp = await client.PostAsJsonAsync(url, body);
        Logger.Response((int)resp.StatusCode, url);
        return resp;
    }

    /// <summary>
    /// PUT + JSON + 日志
    /// </summary>
    public static async Task<HttpResponseMessage> PutAsJsonAsync<T>(HttpClient client, string url, T body)
    {
        Logger.Request("PUT", url);
        var resp = await client.PutAsJsonAsync(url, body);
        Logger.Response((int)resp.StatusCode, url);
        return resp;
    }

    /// <summary>
    /// DELETE + 日志
    /// </summary>
    public static async Task<HttpResponseMessage> DeleteAsync(HttpClient client, string url)
    {
        Logger.Request("DELETE", url);
        var resp = await client.DeleteAsync(url);
        Logger.Response((int)resp.StatusCode, url);
        return resp;
    }

    /// <summary>
    /// POST（无 Body）+ 日志
    /// </summary>
    public static async Task<HttpResponseMessage> PostAsync(HttpClient client, string url)
    {
        Logger.Request("POST", url);
        var resp = await client.PostAsync(url, null);
        Logger.Response((int)resp.StatusCode, url);
        return resp;
    }
}
