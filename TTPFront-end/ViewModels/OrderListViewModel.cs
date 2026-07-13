using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;
using System.Windows;
using TTPFrontEnd.Models;
using TTPFrontEnd.Utils;

namespace TTPFrontEnd.ViewModels;

public partial class OrderListViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;
    public OrderListViewModel(HttpClient httpClient) { _httpClient = httpClient; }

    [ObservableProperty] private ObservableCollection<OrderInfo> _orders = new();
    [ObservableProperty] private OrderInfo? _selectedOrder;
    [ObservableProperty] private string _filterStatus = "全部";
    [ObservableProperty] private bool _isLoading = false;
    [ObservableProperty] private string _errorMessage = string.Empty;

    public List<string> StatusOptions { get; } = new() { "全部", "待支付", "已支付", "已退票", "已改签", "已取消" };

    partial void OnFilterStatusChanged(string value) { _ = LoadAsync(); }

    public async Task LoadAsync()
    {
        IsLoading = true; ErrorMessage = string.Empty;
        try
        {
            string url = "/api/orders";
            if (FilterStatus != "全部") { var key = FilterStatus switch { "待支付" => "unpaid", "已支付" => "paid", "已退票" => "refunded", "已改签" => "changed", "已取消" => "cancelled", _ => "" }; url += $"?status={key}"; }
            var response = await HttpLogger.GetAsync(_httpClient, url);
            if (response.IsSuccessStatusCode) { var result = await response.Content.ReadFromJsonAsync<ApiResponse<List<OrderInfo>>>(); if (result?.Data != null) { Orders.Clear(); foreach (var o in result.Data) Orders.Add(o); } }
            else { var error = await response.Content.ReadFromJsonAsync<ApiResponse<object>>(); var msg = error?.Message ?? "加载失败"; ErrorMessage = msg; Logger.Warn($"订单列表加载失败: {msg}"); }
        }
        catch (HttpRequestException ex) { Logger.Error($"订单列表网络异常: {ex.Message}"); ErrorMessage = "无法连接服务器"; }
        catch (Exception ex) { Logger.Error($"订单列表加载异常: {ex}"); ErrorMessage = ex.Message; }
        finally { IsLoading = false; }
    }

    [RelayCommand] private void Refund(OrderInfo? order) { if (order == null || order.Status != "paid") return; var w = new Views.RefundWindow(_httpClient, order); WindowHelper.SafeSetOwner(w); if (w.ShowDialog() == true) _ = LoadAsync(); }
    [RelayCommand] private void ChangeTicket(OrderInfo? order) { if (order == null || order.Status != "paid") return; var w = new Views.ChangeTicketWindow(_httpClient, order); WindowHelper.SafeSetOwner(w); if (w.ShowDialog() == true) _ = LoadAsync(); }
}