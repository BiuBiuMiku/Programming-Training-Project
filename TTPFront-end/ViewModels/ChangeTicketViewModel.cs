using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;
using System.Windows;
using TTPFrontEnd.Models;
using TTPFrontEnd.Utils;

namespace TTPFrontEnd.ViewModels;

public partial class ChangeTicketViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;
    private readonly OrderInfo _order;
    public ChangeTicketViewModel(HttpClient httpClient, OrderInfo order) { _httpClient = httpClient; _order = order; OrderId = order.OrderId; OldTrainNo = order.TrainNo; FromStation = order.FromStation; ToStation = order.ToStation; StepTitle = "查询可改签车次..."; }

    [ObservableProperty] private string _orderId; [ObservableProperty] private string _oldTrainNo; [ObservableProperty] private string _fromStation; [ObservableProperty] private string _toStation;
    [ObservableProperty] private string _stepTitle; [ObservableProperty] private ObservableCollection<ChangeTrainOption> _availableTrains = new();
    [ObservableProperty] private ChangeTrainOption? _selectedTrain; [ObservableProperty] private SeatTypeInfo? _selectedSeatType;
    [ObservableProperty] private bool _optionsLoaded = false; [ObservableProperty] private bool _changeDone = false;
    [ObservableProperty] private bool _isLoading = false; [ObservableProperty] private string _errorMessage = string.Empty; [ObservableProperty] private string _resultMessage = string.Empty;

    public async Task LoadOptionsAsync()
    {
        IsLoading = true;
        try
        {
            var r = await HttpLogger.GetAsync(_httpClient, $"/api/orders/{_order.OrderId}/change-options?date={_order.Date}");
            if (r.IsSuccessStatusCode) { var d = await r.Content.ReadFromJsonAsync<ApiResponse<ChangeOptionsResult>>(); if (d?.Data?.AvailableTrains != null) { AvailableTrains.Clear(); foreach (var t in d.Data.AvailableTrains) AvailableTrains.Add(t); OptionsLoaded = true; StepTitle = $"选择新车次（共 {AvailableTrains.Count} 趟可选）"; } }
            else { var e = await r.Content.ReadFromJsonAsync<ApiResponse<object>>(); var msg = e?.Message ?? "查询失败"; ErrorMessage = msg; Logger.Warn($"改签可选车次查询失败 [{_order.OrderId}]: {msg}"); }
        }
        catch (Exception ex) { Logger.Error($"改签可选车次查询异常 [{_order.OrderId}]: {ex}"); ErrorMessage = ex.Message; }
        finally { IsLoading = false; }
    }

    private bool CanConfirm => !IsLoading && SelectedTrain != null && SelectedSeatType != null;
    partial void OnSelectedTrainChanged(ChangeTrainOption? value) { SelectedSeatType = value?.SeatTypes?.FirstOrDefault(); ConfirmChangeCommand.NotifyCanExecuteChanged(); }
    partial void OnSelectedSeatTypeChanged(SeatTypeInfo? value) => ConfirmChangeCommand.NotifyCanExecuteChanged();
    partial void OnIsLoadingChanged(bool value) => ConfirmChangeCommand.NotifyCanExecuteChanged();

    [RelayCommand(CanExecute = nameof(CanConfirm))]
    private async Task ConfirmChangeAsync(Window? window)
    {
        ErrorMessage = string.Empty; IsLoading = true;
        try
        {
            var payload = new ChangeRequest { NewTrainNo = SelectedTrain!.TrainNo, NewDate = _order.Date, NewSeatType = SelectedSeatType!.Type };
            var r = await HttpLogger.PostAsJsonAsync(_httpClient, $"/api/orders/{_order.OrderId}/change", payload);
            if (r.IsSuccessStatusCode) { var d = await r.Content.ReadFromJsonAsync<ApiResponse<OrderInfo>>(); ChangeDone = true; StepTitle = "改签成功"; ResultMessage = d?.Data != null ? $"已改签为 {d.Data.TrainNo} 次\n差价 ¥{d.Data.TotalPrice - _order.TotalPrice:+0.00;-0.00}" : "改签成功"; }
            else { var e = await r.Content.ReadFromJsonAsync<ApiResponse<object>>(); var msg = e?.Message ?? "改签失败"; ErrorMessage = msg; Logger.Warn($"改签失败 [{_order.OrderId} → {SelectedTrain!.TrainNo}]: {msg}"); }
        }
        catch (Exception ex) { Logger.Error($"改签异常 [{_order.OrderId} → {SelectedTrain?.TrainNo}]: {ex}"); ErrorMessage = ex.Message; }
        finally { IsLoading = false; }
    }

    [RelayCommand] private void Close(Window? window) { if (ChangeDone) window!.DialogResult = true; window?.Close(); }
}