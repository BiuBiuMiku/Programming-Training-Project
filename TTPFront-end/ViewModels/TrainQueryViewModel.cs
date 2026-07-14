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
using static TTPFrontEnd.Utils.WindowHelper;

namespace TTPFrontEnd.ViewModels;

public partial class TrainQueryViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;
    public TrainQueryViewModel(HttpClient httpClient) { _httpClient = httpClient; SearchDate = DateTime.Now; }

    [ObservableProperty] private ObservableCollection<StationInfo> _stations = new();
    [ObservableProperty] private StationInfo? _selectedFromStation;
    [ObservableProperty] private StationInfo? _selectedToStation;
    [ObservableProperty] private DateTime _searchDate;
    [ObservableProperty] private bool _sortByPrice = false;
    [ObservableProperty] private ObservableCollection<RouteOption> _allRoutes = new();
    [ObservableProperty] private RouteOption? _selectedRoute;
    [ObservableProperty] private bool _isLoading = false;
    [ObservableProperty] private string _errorMessage = string.Empty;
    [ObservableProperty] private string _infoMessage = string.Empty;
    [ObservableProperty] private bool _hasResults = false;
    public List<SavedPassenger> SavedPassengers { get; set; } = new();

    private bool CanSearch => !IsLoading;
    partial void OnIsLoadingChanged(bool value) => SearchCommand.NotifyCanExecuteChanged();

    public async Task LoadStationsAsync()
    {
        try
        {
            var r = await HttpLogger.GetAsync(_httpClient, "/api/stations");
            if (r.IsSuccessStatusCode)
            {
                var d = await r.Content.ReadFromJsonAsync<ApiResponse<List<StationInfo>>>();
                if (d?.Data != null) { Stations.Clear(); foreach (var s in d.Data) Stations.Add(s); }
                else Logger.Warn("站点列表 API 返回 Data 为 null");
            }
            else Logger.Warn($"站点列表 API 返回 {r.StatusCode}");
        }
        catch (Exception ex) { Logger.Error($"加载站点列表失败: {ex.Message}"); }
    }

    [RelayCommand(CanExecute = nameof(CanSearch))]
    private async Task SearchAsync()
    {
        ErrorMessage = string.Empty; InfoMessage = string.Empty; IsLoading = true;
        try
        {
            var ds = SearchDate.ToString("yyyy-MM-dd");
            var hasFrom = SelectedFromStation != null;
            var hasTo = SelectedToStation != null;
            var hasBoth = hasFrom && hasTo;

            // 动态构建查询参数
            var qp = new List<string> { $"date={ds}" };
            if (hasFrom) qp.Add($"from={SelectedFromStation!.Code}");
            if (hasTo) qp.Add($"to={SelectedToStation!.Code}");
            var qs = string.Join("&", qp);

            // 并发请求：直达（始终发起）；换乘仅在指定了起终点时发起
            var tasks = new List<Task<HttpResponseMessage>>();
            var dt = HttpLogger.GetAsync(_httpClient, $"/api/trains?{qs}");
            tasks.Add(dt);
            Task<HttpResponseMessage>? tt = null;
            if (hasBoth)
            {
                tt = HttpLogger.GetAsync(_httpClient, $"/api/tickets/transfer?{qs}");
                tasks.Add(tt);
            }
            await Task.WhenAll(tasks);

            var routes = new List<RouteOption>();

            // 直达结果
            if (dt.Result.IsSuccessStatusCode)
            {
                var d = await dt.Result.Content.ReadFromJsonAsync<ApiResponse<List<TrainInfo>>>();
                if (d?.Data != null)
                    foreach (var t in d.Data)
                    {
                        var mp = t.SeatTypes.Count > 0 ? t.SeatTypes.Min(s => s.Price) : 0;
                        var dist = t.TotalDistanceKm > 0 ? t.TotalDistanceKm : 1;
                        routes.Add(new RouteOption
                        {
                            Type = "直达", DisplayTrainNo = t.TrainNo,
                            FromStation = t.FromStation, ToStation = t.ToStation,
                            DepartureTime = t.DepartureTime, ArrivalTime = t.ArrivalTime,
                            Duration = t.Duration,
                            MinPrice = mp * dist, TotalPrice = mp * dist,
                            TotalDistanceKm = dist, IsTransfer = false,
                            SeatTypes = t.SeatTypes, SourceTrain = t
                        });
                    }
            }

            // 换乘结果（仅双站查询时）
            if (hasBoth && tt != null && tt.Result.IsSuccessStatusCode)
            {
                var d = await tt.Result.Content.ReadFromJsonAsync<ApiResponse<TransferResult>>();
                if (d?.Data != null)
                    foreach (var r in d.Data.TransferRoutes)
                    {
                        // 为每段行程生成可选座位类型
                        foreach (var leg in r.Legs)
                            leg.SeatOptions = SeatRate.GenerateOptions(leg);

                        routes.Add(new RouteOption
                        {
                            Type = "中转",
                            DisplayTrainNo = r.Legs.Count >= 2 ? $"{r.Legs[0].TrainNo} → {r.Legs[1].TrainNo}" : "中转",
                            FromStation = r.Legs.Count > 0 ? r.Legs[0].FromStation : SelectedFromStation?.Name ?? "",
                            ToStation = r.Legs.Count >= 2 ? r.Legs[1].ToStation : SelectedToStation?.Name ?? "",
                            DepartureTime = r.Legs.Count > 0 ? r.Legs[0].DepartureTime : "",
                            ArrivalTime = r.Legs.Count >= 2 ? r.Legs[1].ArrivalTime : "",
                            Duration = r.TotalDuration, MinPrice = r.TotalPrice,
                            TotalPrice = r.TotalPrice, IsTransfer = true,
                            TransferStation = r.TransferStation, Legs = r.Legs,
                            SeatTypes = null, TransferRouteData = r
                        });
                    }
            }

            AllRoutes.Clear(); foreach (var r in routes) AllRoutes.Add(r);
            ApplySort(); HasResults = AllRoutes.Count > 0;

            // 信息文本（自动根据是否指定了双站显示换乘数量）
            var dc = routes.Count(r => !r.IsTransfer);
            var tc = routes.Count(r => r.IsTransfer);
            InfoMessage = HasResults
                ? hasBoth
                    ? $"直达 {dc} 趟，中转 {tc} 个 · 共 {AllRoutes.Count} 个方案"
                    : $"共 {AllRoutes.Count} 趟车次"
                : "未找到出行方案，请调整条件";
        }
        catch (HttpRequestException ex) { Logger.Error($"车次查询网络异常: {ex.Message}"); ErrorMessage = "无法连接服务器，请检查网络"; }
        catch (Exception ex) { Logger.Error($"车次查询异常: {ex}"); ErrorMessage = $"查询失败: {ex.Message}"; }
        finally { IsLoading = false; }
    }

    [RelayCommand] private void SortByTime() { SortByPrice = false; ApplySort(); }
    [RelayCommand] private void SortByPriceAsc() { SortByPrice = true; ApplySort(); }
    private static int ParseDurationMinutes(string d)
    {
        if (string.IsNullOrEmpty(d)) return int.MaxValue;
        var mins = 0;
        var hIdx = d.IndexOf('h'); if (hIdx > 0 && int.TryParse(d[..hIdx], out var h)) mins += h * 60;
        var mIdx = d.IndexOf('m', hIdx + 1); if (mIdx > 0) { var start = d[..mIdx].LastIndexOfAny(new[] { 'h', ' ' }) + 1; if (int.TryParse(d[start..mIdx], out var m)) mins += m; }
        return mins > 0 ? mins : int.MaxValue;
    }
    private void ApplySort()
    {
        var s = SortByPrice
            ? AllRoutes.OrderBy(r => r.MinPrice).ThenBy(r => ParseDurationMinutes(r.Duration)).ToList()
            : AllRoutes.OrderBy(r => ParseDurationMinutes(r.Duration)).ThenBy(r => r.MinPrice).ToList();
        AllRoutes.Clear(); foreach (var r in s) AllRoutes.Add(r);
    }
    [RelayCommand] private void SwapStations() { (SelectedFromStation, SelectedToStation) = (SelectedToStation, SelectedFromStation); }

    [RelayCommand]
    private async Task ViewDetailAsync(RouteOption? r)
    {
        if (r == null || r.IsTransfer) return;
        try
        {
            var date = SearchDate.ToString("yyyy-MM-dd");
            Logger.Info($"打开车次详情: {r.DisplayTrainNo}, date={date}");
            var w = new Views.TrainDetailWindow(_httpClient, r.DisplayTrainNo, date);
            SafeSetOwner(w);
            w.ShowDialog();
        }
        catch (Exception ex)
        {
            Logger.Error($"打开车次详情失败: {ex}");
            ErrorMessage = $"打开详情失败: {ex.Message}";
        }
    }

    [RelayCommand]
    private void BuyTicket(RouteOption? r)
    {
        if (r == null) return;
        try
        {
            Views.BookingWindow w;
            if (r.IsTransfer && r.TransferRouteData != null)
                w = new Views.BookingWindow(_httpClient, r.TransferRouteData, SearchDate.ToString("yyyy-MM-dd"), SavedPassengers);
            else if (!r.IsTransfer && r.SourceTrain != null)
                w = new Views.BookingWindow(_httpClient, r.SourceTrain.TrainNo, r.SourceTrain.FromStation, r.SourceTrain.ToStation, SearchDate.ToString("yyyy-MM-dd"), r.SourceTrain.SeatTypes, SavedPassengers, r.TotalDistanceKm);
            else return;
            SafeSetOwner(w);
            var result = w.ShowDialog();
            if (result == true) _ = SearchAsync();
        }
        catch (Exception ex)
        {
            Logger.Error($"打开购票窗口失败: {ex}");
            ErrorMessage = $"打开购票窗口失败: {ex.Message}";
        }
    }
}