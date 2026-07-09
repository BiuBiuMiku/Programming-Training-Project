using CommunityToolkit.Mvvm.ComponentModel;
using System;
using System.Collections.ObjectModel;
using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;
using TTPFrontEnd.Models;
using TTPFrontEnd.Utils;

namespace TTPFrontEnd.ViewModels;

public partial class TrainDetailViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;
    public TrainDetailViewModel(HttpClient httpClient, string trainNo, string date) { _httpClient = httpClient; TrainNo = trainNo; Date = date; }

    [ObservableProperty] private string _trainNo;
    [ObservableProperty] private string _date;
    [ObservableProperty] private string _fromStation = string.Empty;
    [ObservableProperty] private string _toStation = string.Empty;
    [ObservableProperty] private string _departureTime = string.Empty;
    [ObservableProperty] private string _arrivalTime = string.Empty;
    [ObservableProperty] private string _duration = string.Empty;
    [ObservableProperty] private ObservableCollection<TrainStop> _stops = new();
    [ObservableProperty] private ObservableCollection<SeatTypeInfo> _seatTypes = new();
    [ObservableProperty] private bool _isLoading = true;
    [ObservableProperty] private string _errorMessage = string.Empty;

    public async Task LoadDetailAsync()
    {
        IsLoading = true; ErrorMessage = string.Empty;
        try
        {
            var r = await HttpLogger.GetAsync(_httpClient, $"/api/trains/{TrainNo}?date={Date}");
            if (r.IsSuccessStatusCode)
            {
                var d = await r.Content.ReadFromJsonAsync<ApiResponse<TrainDetailInfo>>();
                if (d?.Data != null)
                {
                    var t = d.Data;
                    FromStation = t.FromStation;
                    ToStation = t.ToStation;
                    DepartureTime = t.DepartureTime;
                    ArrivalTime = t.ArrivalTime;
                    Duration = t.Duration;
                    Stops.Clear();
                    foreach (var s in t.Stops) Stops.Add(s);
                    SeatTypes.Clear();
                    foreach (var s in t.SeatTypes) SeatTypes.Add(s);
                }
            }
            else
            {
                var e = await r.Content.ReadFromJsonAsync<ApiResponse<object>>();
                ErrorMessage = e?.Message ?? "加载失败";
            }
        }
        catch (Exception ex) { Logger.Error($"车次详情加载异常 [{TrainNo}]: {ex}"); ErrorMessage = $"加载失败: {ex.Message}"; }
        finally { IsLoading = false; }
    }
}