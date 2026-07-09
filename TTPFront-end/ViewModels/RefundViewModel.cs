using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;
using System.Windows;
using TTPFrontEnd.Models;
using TTPFrontEnd.Utils;

namespace TTPFrontEnd.ViewModels;

public partial class RefundViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;
    private readonly OrderInfo _order;
    public RefundViewModel(HttpClient httpClient, OrderInfo order) { _httpClient = httpClient; _order = order; OrderId = order.OrderId; TrainNo = order.TrainNo; StepTitle = "查询退票费率..."; }

    [ObservableProperty] private string _orderId; [ObservableProperty] private string _trainNo; [ObservableProperty] private string _stepTitle;
    [ObservableProperty] private double _originalPrice; [ObservableProperty] private double _fee; [ObservableProperty] private double _refundAmount;
    [ObservableProperty] private string _feeRate = string.Empty; [ObservableProperty] private string _reason = string.Empty;
    [ObservableProperty] private bool _feeLoaded = false; [ObservableProperty] private bool _refundDone = false;
    [ObservableProperty] private bool _isLoading = false; [ObservableProperty] private string _errorMessage = string.Empty;

    public async Task LoadFeeAsync()
    {
        IsLoading = true;
        try
        {
            var response = await HttpLogger.GetAsync(_httpClient, $"/api/orders/{_order.OrderId}/refund-fee");
            if (response.IsSuccessStatusCode) { var r = await response.Content.ReadFromJsonAsync<ApiResponse<RefundFeeInfo>>(); if (r?.Data != null) { var f = r.Data; OriginalPrice = f.OriginalPrice; Fee = f.Fee; RefundAmount = f.RefundAmount; FeeRate = f.FeeRate; Reason = f.Reason; FeeLoaded = true; StepTitle = "确认退票"; } }
            else { var e = await response.Content.ReadFromJsonAsync<ApiResponse<object>>(); var msg = e?.Message ?? "无法查询退票费率"; ErrorMessage = msg; Logger.Warn($"退票费率查询失败 [{_order.OrderId}]: {msg}"); }
        }
        catch (Exception ex) { Logger.Error($"退票费率查询异常 [{_order.OrderId}]: {ex}"); ErrorMessage = ex.Message; }
        finally { IsLoading = false; }
    }

    [RelayCommand]
    private async Task ConfirmRefundAsync(Window? window)
    {
        ErrorMessage = string.Empty; IsLoading = true;
        try
        {
            var r = await HttpLogger.PostAsJsonAsync(_httpClient, $"/api/orders/{_order.OrderId}/refund", new { reason = "用户申请退票" });
            if (r.IsSuccessStatusCode) { RefundDone = true; StepTitle = "退票成功"; }
            else { var e = await r.Content.ReadFromJsonAsync<ApiResponse<object>>(); var msg = e?.Message ?? "退票失败"; ErrorMessage = msg; Logger.Warn($"退票失败 [{_order.OrderId}]: {msg}"); }
        }
        catch (Exception ex) { Logger.Error($"退票异常 [{_order.OrderId}]: {ex}"); ErrorMessage = ex.Message; }
        finally { IsLoading = false; }
    }

    [RelayCommand] private void Close(Window? window) { if (RefundDone) window!.DialogResult = true; window?.Close(); }
}