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

public partial class TrainEditViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;
    private readonly List<StationInfo> _allStations;
    private readonly string? _existingTrainNo;
    private readonly Dictionary<(string, string), int> _distanceLookup;

    public TrainEditViewModel(HttpClient httpClient, List<StationInfo> stations, string? existingTrainNo)
    {
        _httpClient = httpClient; _allStations = stations; _existingTrainNo = existingTrainNo;
        IsNewMode = existingTrainNo == null; Title = IsNewMode ? "新增车次" : $"编辑车次 {existingTrainNo}";
        _distanceLookup = new();
        foreach (var s in stations) foreach (var e in s.Edges) { var k = (e.FromCode, e.ToCode); if (!_distanceLookup.ContainsKey(k)) _distanceLookup[k] = e.Distance; }
    }

    [ObservableProperty] private string _title; [ObservableProperty] private bool _isNewMode;
    [ObservableProperty] private string _trainNo = string.Empty; [ObservableProperty] private string _fromStation = string.Empty; [ObservableProperty] private string _toStation = string.Empty;
    [ObservableProperty] private string _departureTime = "08:00"; [ObservableProperty] private string _arrivalTime = "12:00"; [ObservableProperty] private string _duration = string.Empty;
    [ObservableProperty] private ObservableCollection<EditableStop> _stops = new(); [ObservableProperty] private EditableStop? _selectedStop;
    [ObservableProperty] private ObservableCollection<EditableSeatType> _seatTypes = new(); [ObservableProperty] private EditableSeatType? _selectedSeatType;
    [ObservableProperty] private bool _isLoading = false; [ObservableProperty] private string _errorMessage = string.Empty;

    public List<string> StationNames => _allStations.Select(s => s.Name).ToList();

    private bool CanSave => !IsLoading && !string.IsNullOrWhiteSpace(TrainNo) && !string.IsNullOrWhiteSpace(FromStation) && !string.IsNullOrWhiteSpace(ToStation) && Stops.Count >= 2 && SeatTypes.Count > 0;
    partial void OnTrainNoChanged(string value) => SaveCommand.NotifyCanExecuteChanged();
    partial void OnFromStationChanged(string value) => SaveCommand.NotifyCanExecuteChanged();
    partial void OnToStationChanged(string value) => SaveCommand.NotifyCanExecuteChanged();
    partial void OnIsLoadingChanged(bool value) => SaveCommand.NotifyCanExecuteChanged();

    private string? GetCodeByName(string? n) => string.IsNullOrWhiteSpace(n) ? null : _allStations.FirstOrDefault(s => s.Name == n || s.Code == n)?.Code;
    private int LookupDistance(string? a, string? b) { if (a == null || b == null) return 0; if (_distanceLookup.TryGetValue((a, b), out var d)) return d; if (_distanceLookup.TryGetValue((b, a), out var d2)) return d2; return 0; }
    private void RecalculateDistances() { for (int i = 0; i < Stops.Count; i++) { if (i == 0) { Stops[i].DistanceKm = 0; continue; } Stops[i].DistanceKm = LookupDistance(GetCodeByName(Stops[i - 1].Station), GetCodeByName(Stops[i].Station)); } }

    [RelayCommand] private void AddStop() { Stops.Add(new EditableStop { Station = "", Arrive = "", Depart = "", Seq = Stops.Count + 1 }); }
    [RelayCommand] private void RemoveStop(EditableStop? s) { if (s != null) { Stops.Remove(s); RecalculateDistances(); } }
    [RelayCommand] private void MoveStopUp(EditableStop? s) { var i = Stops.IndexOf(s!); if (i > 0) { Stops.Move(i, i - 1); RecalculateDistances(); } }
    [RelayCommand] private void MoveStopDown(EditableStop? s) { var i = Stops.IndexOf(s!); if (i < Stops.Count - 1) { Stops.Move(i, i + 1); RecalculateDistances(); } }
    public void OnStopStationChanged() { RecalculateDistances(); }

    [RelayCommand] private void AddSeatType() { SeatTypes.Add(new EditableSeatType()); }
    [RelayCommand] private void RemoveSeatType(EditableSeatType? s) { if (s != null) SeatTypes.Remove(s); }

    public async Task LoadExistingAsync()
    {
        if (_existingTrainNo == null) return; IsLoading = true;
        try
        {
            var r = await HttpLogger.GetAsync(_httpClient, $"/api/trains/{_existingTrainNo}");
            if (r.IsSuccessStatusCode) { var d = await r.Content.ReadFromJsonAsync<ApiResponse<TrainDetailInfo>>(); if (d?.Data != null) { var t = d.Data; TrainNo = t.TrainNo; FromStation = t.FromStation; ToStation = t.ToStation; DepartureTime = t.DepartureTime; ArrivalTime = t.ArrivalTime; Duration = t.Duration; Stops.Clear(); foreach (var s in t.Stops) Stops.Add(new EditableStop { Station = s.Station, Arrive = s.Arrive, Depart = s.Depart, DistanceKm = s.DistanceKm }); SeatTypes.Clear(); foreach (var s in t.SeatTypes) SeatTypes.Add(new EditableSeatType { Type = s.Type, Price = s.Price, Remain = s.Remain }); } }
            else Logger.Warn($"加载车次详情失败 [{_existingTrainNo}]: {r.StatusCode}");
        }
        catch (Exception ex) { Logger.Error($"加载车次详情异常 [{_existingTrainNo}]: {ex}"); ErrorMessage = $"加载失败: {ex.Message}"; }
        finally { IsLoading = false; }
    }

    [RelayCommand(CanExecute = nameof(CanSave))]
    private async Task SaveAsync(Window? window)
    {
        ErrorMessage = string.Empty; IsLoading = true;
        try
        {
            var payload = new { trainNo = TrainNo, fromStation = FromStation, toStation = ToStation, departureTime = DepartureTime, arrivalTime = ArrivalTime, duration = Duration, stops = Stops.Select((s, i) => new { station = s.Station, arrive = s.Arrive, depart = s.Depart, seq = i + 1, distanceKm = s.DistanceKm }), seatTypes = SeatTypes.Select(st => new { type = st.Type, price = st.Price, remain = st.Remain }) };
            var r = IsNewMode ? await HttpLogger.PostAsJsonAsync(_httpClient, "/api/admin/trains", payload) : await HttpLogger.PutAsJsonAsync(_httpClient, $"/api/admin/trains/{_existingTrainNo}", payload);
            if (r.IsSuccessStatusCode) { window!.DialogResult = true; window.Close(); }
            else { var e = await r.Content.ReadFromJsonAsync<ApiResponse<object>>(); var msg = e?.Message ?? "保存失败"; ErrorMessage = msg; Logger.Warn($"车次保存失败 [{TrainNo}]: {msg}"); }
        }
        catch (HttpRequestException ex) { Logger.Error($"车次保存网络异常 [{TrainNo}]: {ex.Message}"); ErrorMessage = "无法连接服务器"; }
        catch (Exception ex) { Logger.Error($"车次保存异常 [{TrainNo}]: {ex}"); ErrorMessage = $"保存失败: {ex.Message}"; }
        finally { IsLoading = false; }
    }

    [RelayCommand] private void Cancel(Window? window) => window?.Close();
}

public partial class EditableStop : ObservableObject
{
    [ObservableProperty] private string _station = string.Empty;
    [ObservableProperty] private string _arrive = string.Empty;
    [ObservableProperty] private string _depart = string.Empty;
    [ObservableProperty] private int _seq;
    [ObservableProperty] private int _distanceKm;
    public Action? OnStationChangedExternal { get; set; }
    partial void OnStationChanged(string value) { OnStationChangedExternal?.Invoke(); }
}

public partial class EditableSeatType : ObservableObject
{
    [ObservableProperty] private string _type = string.Empty;
    [ObservableProperty] private double _price;
    [ObservableProperty] private int _remain = 100;
}
