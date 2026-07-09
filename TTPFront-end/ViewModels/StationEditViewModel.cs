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

public partial class StationEditViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;
    private readonly List<StationInfo> _existingStations;
    private readonly string? _editingCode;

    public StationEditViewModel(HttpClient httpClient, List<StationInfo> existingStations, string? editingCode = null)
    {
        _httpClient = httpClient;
        _existingStations = existingStations;
        _editingCode = editingCode;
        IsNewMode = editingCode == null;
        Title = IsNewMode ? "新增站点" : $"编辑站点 {editingCode}";
        foreach (var s in existingStations)
        {
            if (s.Code == editingCode) continue;
            Distances.Add(new EditableDistance { StationCode = s.Code, StationName = $"{s.Name} ({s.Code})", Distance = 0 });
        }
    }

    [ObservableProperty] private string _title;
    [ObservableProperty] private bool _isNewMode;
    [ObservableProperty] private string _stationName = string.Empty;
    [ObservableProperty] private string _stationCode = string.Empty;
    [ObservableProperty] private string _stationCity = string.Empty;
    [ObservableProperty] private ObservableCollection<EditableDistance> _distances = new();
    [ObservableProperty] private bool _isLoading = false;
    [ObservableProperty] private string _errorMessage = string.Empty;

    private bool CanSave => !IsLoading && !string.IsNullOrWhiteSpace(StationCode) && !string.IsNullOrWhiteSpace(StationName);
    partial void OnStationCodeChanged(string value) => SaveCommand.NotifyCanExecuteChanged();
    partial void OnStationNameChanged(string value) => SaveCommand.NotifyCanExecuteChanged();
    partial void OnIsLoadingChanged(bool value) => SaveCommand.NotifyCanExecuteChanged();

    [RelayCommand(CanExecute = nameof(CanSave))]
    private async Task SaveAsync(Window? window)
    {
        ErrorMessage = string.Empty; IsLoading = true;
        try
        {
            var edges = Distances.Where(d => d.Distance > 0).Select(d => new { fromCode = StationCode, toCode = d.StationCode, distance = d.Distance }).ToList();
            var payload = new { code = StationCode, name = StationName, city = StationCity, edges };
            var r = IsNewMode ? await HttpLogger.PostAsJsonAsync(_httpClient, "/api/admin/stations", payload) : await HttpLogger.PutAsJsonAsync(_httpClient, $"/api/admin/stations/{_editingCode}", payload);
            if (r.IsSuccessStatusCode) 
            {
                window!.DialogResult = true;
                window.Close();
            }
            else
            {
                var e = await r.Content.ReadFromJsonAsync<ApiResponse<object>>();
                var msg = e?.Message ?? "保存失败";
                ErrorMessage = msg;
                Logger.Warn($"站点保存失败 [{StationCode}]: {msg}");
            }
        }
        catch (HttpRequestException ex) { Logger.Error($"站点保存网络异常 [{StationCode}]: {ex.Message}"); ErrorMessage = "无法连接服务器"; }
        catch (Exception ex) { Logger.Error($"站点保存异常 [{StationCode}]: {ex}"); ErrorMessage = $"保存失败: {ex.Message}"; }
        finally { IsLoading = false; }
    }

    [RelayCommand] private void Cancel(Window? window) => window?.Close();
}

public partial class EditableDistance : ObservableObject
{
    public string StationCode { get; set; } = string.Empty;
    public string StationName { get; set; } = string.Empty;
    [ObservableProperty] private int _distance;
}