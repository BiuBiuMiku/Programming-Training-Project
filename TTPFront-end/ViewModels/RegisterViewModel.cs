using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;
using System.Timers;
using System.Windows;
using TTPFrontEnd.Utils;

namespace TTPFrontEnd.ViewModels;

public partial class RegisterViewModel : ObservableObject, IDisposable
{
    private readonly HttpClient _httpClient;
    private Timer? _countdownTimer;
    private int _countdownSeconds;

    public RegisterViewModel(HttpClient httpClient) { _httpClient = httpClient; }

    [ObservableProperty] private string _phoneNumber = string.Empty;
    [ObservableProperty] private string _verifyCode = string.Empty;
    [ObservableProperty] private string _password = string.Empty;
    [ObservableProperty] private string _confirmPassword = string.Empty;
    [ObservableProperty] private bool _isLoading = false;
    [ObservableProperty] private string _errorMessage = string.Empty;
    [ObservableProperty] private string _successMessage = string.Empty;
    [ObservableProperty] private string _sendCodeButtonText = "获取验证码";
    [ObservableProperty] private bool _canSendCode = true;

    private bool CanSendCodeNow => CanSendCode && !IsLoading && !string.IsNullOrWhiteSpace(PhoneNumber) && PhoneNumber.Length == 11 && System.Text.RegularExpressions.Regex.IsMatch(PhoneNumber, @"^1[3-9]\d{9}$");
    private bool CanRegister => !IsLoading && !string.IsNullOrWhiteSpace(PhoneNumber) && !string.IsNullOrWhiteSpace(VerifyCode) && VerifyCode.Length >= 4 && !string.IsNullOrWhiteSpace(Password) && Password.Length >= 6 && Password == ConfirmPassword;

    partial void OnPhoneNumberChanged(string value) => RefreshCommands();
    partial void OnVerifyCodeChanged(string value) => RefreshCommands();
    partial void OnPasswordChanged(string value) => RefreshCommands();
    partial void OnConfirmPasswordChanged(string value) => RefreshCommands();
    partial void OnIsLoadingChanged(bool value) => RefreshCommands();
    partial void OnCanSendCodeChanged(bool value) => SendCodeCommand.NotifyCanExecuteChanged();

    private void RefreshCommands() { SendCodeCommand.NotifyCanExecuteChanged(); RegisterCommand.NotifyCanExecuteChanged(); }

    [RelayCommand(CanExecute = nameof(CanSendCodeNow))]
    private async Task SendCodeAsync()
    {
        ErrorMessage = string.Empty; IsLoading = true;
        try
        {
            var payload = new { phone = PhoneNumber };
            var response = await HttpLogger.PostAsJsonAsync(_httpClient, "/api/auth/send-code", payload);
            if (response.IsSuccessStatusCode) { SuccessMessage = "验证码已发送，请注意查收"; StartCountdown(60); }
            else { var error = await response.Content.ReadFromJsonAsync<ErrorResponse>(); ErrorMessage = error?.Message ?? $"发送失败 ({response.StatusCode})"; }
        }
        catch (HttpRequestException ex) { Logger.Error($"发送验证码网络异常: {ex.Message}"); ErrorMessage = "无法连接服务器，请检查网络"; }
        catch (TaskCanceledException) { Logger.Warn("发送验证码请求超时"); ErrorMessage = "请求超时，请重试"; }
        catch (Exception ex) { Logger.Error($"发送验证码异常: {ex}"); ErrorMessage = $"未知错误: {ex.Message}"; }
        finally { IsLoading = false; }
    }

    [RelayCommand(CanExecute = nameof(CanRegister))]
    private async Task RegisterAsync()
    {
        ErrorMessage = string.Empty; SuccessMessage = string.Empty; IsLoading = true;
        try
        {
            var payload = new { phone = PhoneNumber, code = VerifyCode, password = Password };
            var response = await HttpLogger.PostAsJsonAsync(_httpClient, "/api/auth/register", payload);
            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<RegisterResponse>();
                if (result?.Token != null)
                {
                    TokenStorage.SaveToken(result.Token, persist: true);
                    var mainWindow = new Views.MainWindow(result.Phone ?? PhoneNumber, "user");
                    mainWindow.Show();
                    Application.Current.Windows[0]?.Close();
                }
                else { SuccessMessage = "注册成功！请返回登录"; await Task.Delay(1500); GoBack(); }
            }
            else { var error = await response.Content.ReadFromJsonAsync<ErrorResponse>(); ErrorMessage = error?.Message ?? $"注册失败 ({response.StatusCode})"; }
        }
        catch (HttpRequestException ex) { Logger.Error($"注册网络异常: {ex.Message}"); ErrorMessage = "无法连接服务器，请检查网络"; }
        catch (TaskCanceledException) { Logger.Warn("注册请求超时"); ErrorMessage = "请求超时，请重试"; }
        catch (Exception ex) { Logger.Error($"注册异常: {ex}"); ErrorMessage = $"未知错误: {ex.Message}"; }
        finally { IsLoading = false; }
    }

    [RelayCommand] private void GoBack() { var w = new Views.LoginWindow(); w.Show(); Application.Current.Windows[0]?.Close(); }

    private void StartCountdown(int s) { _countdownSeconds = s; CanSendCode = false; UpdateCountdownText(); _countdownTimer = new Timer(1000); _countdownTimer.Elapsed += Tick; _countdownTimer.AutoReset = true; _countdownTimer.Start(); }
    private void Tick(object? o, ElapsedEventArgs a) { _countdownSeconds--; if (_countdownSeconds <= 0) { StopCountdown(); return; } Application.Current.Dispatcher.Invoke(() => UpdateCountdownText()); }
    private void UpdateCountdownText() { SendCodeButtonText = $"{_countdownSeconds}s 后重发"; }
    private void StopCountdown() { _countdownTimer?.Stop(); _countdownTimer?.Dispose(); _countdownTimer = null; Application.Current.Dispatcher.Invoke(() => { SendCodeButtonText = "重新获取"; CanSendCode = true; }); }
    public void Dispose() { try { _countdownTimer?.Dispose(); } catch (Exception ex) { Logger.Warn($"RegisterViewModel Dispose 异常: {ex.Message}"); } }

    private class RegisterResponse { public string Token { get; set; } = string.Empty; public string Phone { get; set; } = string.Empty; }
    private class ErrorResponse { public string Message { get; set; } = string.Empty; }
}