using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;
using System.Windows;
using TTPFrontEnd.Utils;

namespace TTPFrontEnd.ViewModels;

public partial class LoginViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;

    public LoginViewModel(HttpClient httpClient)
    {
        _httpClient = httpClient;
    }

    [ObservableProperty] private string _username = string.Empty;
    [ObservableProperty] private string _password = string.Empty;
    [ObservableProperty] private bool _isLoading = false;
    [ObservableProperty] private string _errorMessage = string.Empty;
    [ObservableProperty] private bool _rememberMe = false;

    private bool CanLogin =>
        !IsLoading &&
        !string.IsNullOrWhiteSpace(Username) &&
        !string.IsNullOrWhiteSpace(Password);

    partial void OnUsernameChanged(string value) => LoginCommand.NotifyCanExecuteChanged();
    partial void OnPasswordChanged(string value) => LoginCommand.NotifyCanExecuteChanged();
    partial void OnIsLoadingChanged(bool value) => LoginCommand.NotifyCanExecuteChanged();

    private void OnLoginSuccess(string token, string username, string role)
    {
        TokenStorage.SaveToken(token, RememberMe);
        var mainWindow = new Views.MainWindow(username, role);
        mainWindow.Show();
        Application.Current.Windows[0]?.Close();
    }

    [RelayCommand(CanExecute = nameof(CanLogin))]
    private async Task LoginAsync()
    {
        ErrorMessage = string.Empty;
        IsLoading = true;

        try
        {
            if (Username == "admin" && Password == "1111")
            {
                var mockJwt = GenerateMockToken("admin", "admin");
                Logger.Info("admin mock login");
                OnLoginSuccess(mockJwt, "admin", "admin");
                return;
            }
            if (Username == "user" && Password == "1111")
            {
                var mockJwt = GenerateMockToken("user", "user");
                Logger.Info("user mock login");
                OnLoginSuccess(mockJwt, "user", "user");
                return;
            }

            var payload = new { username = Username, password = Password };
            var response = await HttpLogger.PostAsJsonAsync(_httpClient, "/api/auth/login", payload);

            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<LoginResponse>();
                if (result?.Token != null)
                    OnLoginSuccess(result.Token, result.Username, result.Role);
                else
                    ErrorMessage = "服务器返回数据异常，请重试";
            }
            else
            {
                var error = await response.Content.ReadFromJsonAsync<ErrorResponse>();
                ErrorMessage = error?.Message ?? $"登录失败 ({response.StatusCode})";
            }
        }
        catch (HttpRequestException ex) { Logger.Error($"登录网络异常: {ex.Message}"); ErrorMessage = "无法连接服务器，请检查网络"; }
        catch (TaskCanceledException) { Logger.Warn("登录请求超时"); ErrorMessage = "请求超时，请重试"; }
        catch (Exception ex) { Logger.Error($"登录异常: {ex}"); ErrorMessage = $"未知错误: {ex.Message}"; }
        finally { IsLoading = false; }
    }

    [RelayCommand]
    private void GoToRegister()
    {
        var registerWindow = new Views.RegisterWindow();
        registerWindow.Show();
        Application.Current.Windows[0]?.Close();
    }

    private static string GenerateMockToken(string username, string role)
    {
        var header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
        var now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        var payload = $"{{\"sub\":\"0\",\"username\":\"{username}\",\"role\":\"{role}\",\"iat\":{now},\"exp\":{now + 86400}}}";
        var headerB64 = Convert.ToBase64String(System.Text.Encoding.UTF8.GetBytes(header));
        var payloadB64 = Convert.ToBase64String(System.Text.Encoding.UTF8.GetBytes(payload));
        return $"mock.{headerB64}.{payloadB64}";
    }

    private class LoginResponse
    {
        public string Token { get; set; } = string.Empty;
        public string Username { get; set; } = string.Empty;
        public string Role { get; set; } = "user";
    }
    private class ErrorResponse
    {
        public string Message { get; set; } = string.Empty;
    }
}

public static class TokenStorage
{
    private static readonly string TokenPath = System.IO.Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "TTPFrontEnd", "token.dat");

    public static void SaveToken(string token, bool persist)
    {
        if (!persist) return;
        var dir = System.IO.Path.GetDirectoryName(TokenPath);
        if (!System.IO.Directory.Exists(dir)) System.IO.Directory.CreateDirectory(dir!);
        var bytes = System.Text.Encoding.UTF8.GetBytes(token);
        var encrypted = System.Security.Cryptography.ProtectedData.Protect(
            bytes, null, System.Security.Cryptography.DataProtectionScope.CurrentUser);
        System.IO.File.WriteAllBytes(TokenPath, encrypted);
    }

    public static string? LoadToken()
    {
        if (!System.IO.File.Exists(TokenPath)) return null;
        try
        {
            var encrypted = System.IO.File.ReadAllBytes(TokenPath);
            var bytes = System.Security.Cryptography.ProtectedData.Unprotect(
                encrypted, null, System.Security.Cryptography.DataProtectionScope.CurrentUser);
            return System.Text.Encoding.UTF8.GetString(bytes);
        }
        catch (Exception ex) { Logger.Warn($"加载 Token 失败: {ex.Message}"); return null; }
    }

    public static void ClearToken()
    {
        if (System.IO.File.Exists(TokenPath)) System.IO.File.Delete(TokenPath);
    }
}
