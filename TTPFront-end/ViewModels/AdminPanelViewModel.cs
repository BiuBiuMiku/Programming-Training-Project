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

public partial class AdminPanelViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;

    public AdminPanelViewModel(HttpClient httpClient) { _httpClient = httpClient; }

    [ObservableProperty] private ObservableCollection<StationInfo> _stations = new();
    [ObservableProperty] private StationInfo? _selectedStation;
    [ObservableProperty] private ObservableCollection<TrainInfo> _trains = new();
    [ObservableProperty] private TrainInfo? _selectedTrain;
    [ObservableProperty] private bool _isLoading = false;
    [ObservableProperty] private string _errorMessage = string.Empty;
    [ObservableProperty] private string _successMessage = string.Empty;

    public async Task LoadAsync() { await LoadStationsAsync(); await LoadTrainsAsync(); }

    private async Task LoadStationsAsync()
    {
        try
        {
            var response = await HttpLogger.GetAsync(_httpClient, "/api/stations");
            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<ApiResponse<List<StationInfo>>>();
                if (result?.Data != null) { Stations.Clear(); foreach (var s in result.Data) Stations.Add(s); }
                else Logger.Warn("AdminPanel 站点列表 Data 为 null");
            }
            else Logger.Warn($"AdminPanel 站点列表 API 返回 {response.StatusCode}");
        }
        catch (Exception ex) { Logger.Error($"AdminPanel 加载站点失败: {ex.Message}"); }
    }

    private async Task LoadTrainsAsync()
    {
        IsLoading = true; ErrorMessage = string.Empty;
        try
        {
            var response = await HttpLogger.GetAsync(_httpClient, "/api/admin/trains");
            if (response.IsSuccessStatusCode)
            {
                var result = await response.Content.ReadFromJsonAsync<ApiResponse<List<TrainInfo>>>();
                if (result?.Data != null) { Trains.Clear(); foreach (var t in result.Data) Trains.Add(t); }
            }
            else Logger.Warn($"AdminPanel 车次列表 API 返回 {response.StatusCode}");
        }
        catch (HttpRequestException ex) { Logger.Error($"AdminPanel 车次列表网络异常: {ex.Message}"); ErrorMessage = "无法连接服务器"; }
        catch (Exception ex) { Logger.Error($"AdminPanel 车次列表异常: {ex}"); ErrorMessage = ex.Message; }
        finally { IsLoading = false; }
    }

    [RelayCommand]
    private void AddStation()
    {
        try
        {
            var window = new Views.StationEditWindow(_httpClient, Stations.ToList());
            WindowHelper.SafeSetOwner(window);
            if (window.ShowDialog() == true) _ = LoadStationsAsync();
        }
        catch (Exception ex) { Logger.Error($"AdminPanel AddStation 异常: {ex}"); MessageBox.Show($"打开窗口失败:\n{ex}", "错误", MessageBoxButton.OK, MessageBoxImage.Error); }
    }

    [RelayCommand]
    private void EditStation(StationInfo? station)
    {
        if (station == null) return;
        try
        {
            var window = new Views.StationEditWindow(_httpClient, Stations.ToList(), station.Code);
            WindowHelper.SafeSetOwner(window);
            if (window.ShowDialog() == true) _ = LoadStationsAsync();
        }
        catch (Exception ex) { Logger.Error($"AdminPanel EditStation 异常 [{station.Code}]: {ex}"); MessageBox.Show($"打开窗口失败:\n{ex}", "错误", MessageBoxButton.OK, MessageBoxImage.Error); }
    }

    [RelayCommand]
    private async Task DeleteStationAsync(StationInfo? station)
    {
        if (station == null) return;
        if (MessageBox.Show($"确定要删除站点 {station.Name} ({station.Code}) 吗？\n\n删除站点可能影响使用该站的车次数据。", "确认删除", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes) return;
        try
        {
            var response = await HttpLogger.DeleteAsync(_httpClient, $"/api/admin/stations/{station.Code}");
            if (response.IsSuccessStatusCode) { Stations.Remove(station); SuccessMessage = $"站点 {station.Name} 已删除"; }
            else { var error = await response.Content.ReadFromJsonAsync<ApiResponse<object>>(); var msg = error?.Message ?? "删除失败"; ErrorMessage = msg; Logger.Warn($"删除站点失败 [{station.Code}]: {msg}"); }
        }
        catch (Exception ex) { Logger.Error($"删除站点异常 [{station.Code}]: {ex}"); ErrorMessage = $"删除失败: {ex.Message}"; }
    }

    [RelayCommand]
    private void AddTrain()
    {
        try
        {
            var editWindow = new Views.TrainEditWindow(_httpClient, Stations.ToList(), null);
            WindowHelper.SafeSetOwner(editWindow);
            if (editWindow.ShowDialog() == true) _ = LoadTrainsAsync();
        }
        catch (Exception ex) { Logger.Error($"打开新增车次窗口异常: {ex}"); ErrorMessage = $"打开窗口失败: {ex.Message}"; }
    }

    [RelayCommand]
    private void EditTrain(TrainInfo? train)
    {
        if (train == null) return;
        try
        {
            var editWindow = new Views.TrainEditWindow(_httpClient, Stations.ToList(), train.TrainNo);
            WindowHelper.SafeSetOwner(editWindow);
            if (editWindow.ShowDialog() == true) _ = LoadTrainsAsync();
        }
        catch (Exception ex) { Logger.Error($"AdminPanel EditTrain 异常 [{train.TrainNo}]: {ex}"); ErrorMessage = $"打开窗口失败: {ex.Message}"; }
    }

    [RelayCommand]
    private async Task DeleteTrainAsync(TrainInfo? train)
    {
        if (train == null) return;
        if (MessageBox.Show($"确定要删除车次 {train.TrainNo} 吗？此操作不可恢复。", "确认删除", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes) return;
        try
        {
            var response = await HttpLogger.DeleteAsync(_httpClient, $"/api/admin/trains/{train.TrainNo}");
            if (response.IsSuccessStatusCode) { Trains.Remove(train); SuccessMessage = $"车次 {train.TrainNo} 已删除"; }
            else { var error = await response.Content.ReadFromJsonAsync<ApiResponse<object>>(); var msg = error?.Message ?? "删除失败"; ErrorMessage = msg; Logger.Warn($"删除车次失败 [{train.TrainNo}]: {msg}"); }
        }
        catch (Exception ex) { Logger.Error($"删除车次异常 [{train.TrainNo}]: {ex}"); ErrorMessage = $"删除失败: {ex.Message}"; }
    }
}