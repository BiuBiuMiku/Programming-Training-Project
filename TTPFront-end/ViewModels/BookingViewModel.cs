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

public partial class BookingViewModel : ObservableObject
{
    private readonly HttpClient _httpClient;
    private readonly TransferRoute? _transferRoute;

    // ============ 直达购票 ============
    public BookingViewModel(HttpClient httpClient, string trainNo, string from, string to, string date,
        List<SeatTypeInfo> seats, List<SavedPassenger> sp, int dist)
        : this(httpClient, sp)
    {
        TrainNo = trainNo; FromStation = from; ToStation = to; Date = date; TotalDistanceKm = dist;
        IsTransfer = false; SeatTypes = new ObservableCollection<SeatTypeInfo>(seats);
        SelectedSeatType = SeatTypes.FirstOrDefault();
    }

    // ============ 中转购票 ============
    public BookingViewModel(HttpClient httpClient, TransferRoute route, string date,
        List<SavedPassenger> sp) : this(httpClient, sp)
    {
        _transferRoute = route; IsTransfer = true; Date = date;
        TransferInfo = $"中转站: {route.TransferStation}"; StepTitle = "中转购票 · 请为每段选择座位";

        // 为每段行程构建可选项（默认选中第一个）
        foreach (var leg in route.Legs)
        {
            var opt = new TransferLegOption { Leg = leg };
            foreach (var st in leg.SeatOptions) opt.SeatTypes.Add(st);
            opt.SelectedSeatType = opt.SeatTypes.FirstOrDefault();
            TransferLegOptions.Add(opt);
        }
        RefreshTransferTotal();
    }

    private BookingViewModel(HttpClient httpClient, List<SavedPassenger> sp)
    {
        _httpClient = httpClient;
        foreach (var p in sp) PassengerOptions.Add(new SelectablePassenger { Passenger = p });
        HasSavedPassengers = sp.Count > 0;
    }

    // ============ 基本信息 ============
    [ObservableProperty] private string _trainNo = string.Empty;
    [ObservableProperty] private string _fromStation = string.Empty;
    [ObservableProperty] private string _toStation = string.Empty;
    [ObservableProperty] private string _date = string.Empty;
    [ObservableProperty] private bool _isTransfer;
    [ObservableProperty] private string? _transferInfo;

    // ============ 直达: 座位类型 ============
    [ObservableProperty] private ObservableCollection<SeatTypeInfo> _seatTypes = new();
    [ObservableProperty] private SeatTypeInfo? _selectedSeatType;
    public int TotalDistanceKm { get; private set; }
    public bool HasSeatTypes => !IsTransfer && SeatTypes.Count > 0;

    // ============ 中转: 每段座位 ============
    [ObservableProperty]
    private ObservableCollection<TransferLegOption> _transferLegOptions = new();

    // ============ 乘客 ============
    [ObservableProperty] private ObservableCollection<SelectablePassenger> _passengerOptions = new();
    [ObservableProperty] private bool _hasSavedPassengers;

    // ============ 订单状态 ============
    [ObservableProperty] private bool _orderPlaced = false;
    [ObservableProperty] private string? _orderId;
    [ObservableProperty] private double _totalPrice;
    [ObservableProperty] private bool _paymentDone = false;
    [ObservableProperty] private bool _isLoading = false;
    [ObservableProperty] private string _errorMessage = string.Empty;
    [ObservableProperty] private string _stepTitle = "填写订单信息";

    // ============ 票价计算 ============

    private double CalcSinglePersonPrice()
    {
        if (!IsTransfer)
            return SelectedSeatType != null ? SelectedSeatType.Price * TotalDistanceKm : 0;
        return TransferLegOptions.Sum(o => o.SelectedSeatType?.Price ?? 0);
    }

    public double PricePerPerson => CalcSinglePersonPrice();
    public string PriceDisplay => $"¥ {PricePerPerson:F0} / 人";
    public int SelectedCount => PassengerOptions.Count(p => p.IsSelected);
    public string TotalPriceDisplay => $"¥ {PricePerPerson * SelectedCount:F0}";
    public string DistanceInfo => IsTransfer
        ? $"中转 · {TransferLegOptions.Count} 段行程"
        : $"{TotalDistanceKm} km × {SelectedSeatType?.Price ?? 0:F2} 元/km";

    private void RefreshTransferTotal()
    {
        OnPropertyChanged(nameof(PricePerPerson));
        OnPropertyChanged(nameof(PriceDisplay));
        OnPropertyChanged(nameof(TotalPriceDisplay));
        OnPropertyChanged(nameof(DistanceInfo));
    }

    // ============ 座位类型变更 ============

    partial void OnSelectedSeatTypeChanged(SeatTypeInfo? value)
    {
        OnPropertyChanged(nameof(PriceDisplay));
        OnPropertyChanged(nameof(TotalPriceDisplay));
        OnPropertyChanged(nameof(PricePerPerson));
        OnPropertyChanged(nameof(DistanceInfo));
    }

    // ============ 下单条件 ============

    private bool CanPlaceOrder
    {
        get
        {
            if (IsLoading || !PassengerOptions.Any(p => p.IsSelected)) return false;
            if (IsTransfer) return TransferLegOptions.Count > 0
                && TransferLegOptions.All(o => o.SelectedSeatType != null);
            return SelectedSeatType != null;
        }
    }

    // ============ 直达选座 ============

    [RelayCommand]
    private void SelectSeatType(SeatTypeInfo? st)
    {
        if (st != null) SelectedSeatType = st;
    }

    // ============ 中转段选座 ============

    [RelayCommand]
    private void SelectTransferSeatType(TransferLegOption? opt)
    {
        if (opt == null) return;
        // 取传进来的 SelectedSeatType 使用. 但因为是从 Tag 绑定来的, 需要实际点击触发
        // 这里我们通过 code-behind 的 handler 来设好再调用
    }

    /// <summary>设置某段的座位类型并刷新总价</summary>
    public void SetLegSeatType(TransferLegOption opt, SeatTypeInfo st)
    {
        opt.SelectedSeatType = st;
        RefreshTransferTotal();
        PlaceOrderCommand.NotifyCanExecuteChanged();
    }

    // ============ 下单 ============

    [RelayCommand(CanExecute = nameof(CanPlaceOrder))]
    private async Task PlaceOrderAsync()
    {
        ErrorMessage = string.Empty; IsLoading = true;
        try
        {
            var pax = PassengerOptions.Where(p => p.IsSelected)
                .Select(p => new PassengerInfo { RealName = p.Passenger.RealName, IdCard = p.Passenger.IdCard })
                .ToList();
            if (IsTransfer && _transferRoute != null)
                await PlaceTransferOrders(pax);
            else
                await PlaceDirectOrder(pax);
        }
        catch (HttpRequestException ex) { Logger.Error($"下单网络异常 [{TrainNo}]: {ex.Message}"); ErrorMessage = "无法连接服务器"; }
        catch (Exception ex) { Logger.Error($"下单异常 [{TrainNo}]: {ex}"); ErrorMessage = $"下单失败: {ex.Message}"; }
        finally { IsLoading = false; }
    }

    private async Task PlaceDirectOrder(List<PassengerInfo> pax)
    {
        var r = await HttpLogger.PostAsJsonAsync(_httpClient, "/api/orders",
            new OrderRequest
            {
                TrainNo = TrainNo, Date = Date,
                FromStation = FromStation, ToStation = ToStation,
                SeatType = SelectedSeatType!.Type, Passengers = pax
            });
        if (r.IsSuccessStatusCode)
        {
            var d = await r.Content.ReadFromJsonAsync<ApiResponse<OrderInfo>>();
            if (d?.Data != null) { OrderId = d.Data.OrderId; TotalPrice = d.Data.TotalPrice; OrderPlaced = true; StepTitle = "订单已生成"; }
        }
        else { var e = await r.Content.ReadFromJsonAsync<ApiResponse<object>>(); ErrorMessage = e?.Message ?? "下单失败"; }
    }

    private async Task PlaceTransferOrders(List<PassengerInfo> pax)
    {
        var legs = _transferRoute!.Legs;
        if (legs.Count < 2) { ErrorMessage = "中转路线数据异常"; return; }
        string? fid = null; double tot = 0;
        for (int i = 0; i < legs.Count; i++)
        {
            var leg = legs[i];
            var opt = TransferLegOptions[i];
            var seatType = opt.SelectedSeatType?.Type ?? leg.SeatType;

            var r = await HttpLogger.PostAsJsonAsync(_httpClient, "/api/orders",
                new OrderRequest
                {
                    TrainNo = leg.TrainNo, Date = Date,
                    FromStation = leg.FromStation, ToStation = leg.ToStation,
                    SeatType = seatType, Passengers = pax
                });
            if (!r.IsSuccessStatusCode)
            {
                var e = await r.Content.ReadFromJsonAsync<ApiResponse<object>>();
                ErrorMessage = e?.Message ?? $"第{i + 1}段下单失败"; return;
            }
            var d = await r.Content.ReadFromJsonAsync<ApiResponse<OrderInfo>>();
            if (d?.Data != null) { if (i == 0) fid = d.Data.OrderId; tot += d.Data.TotalPrice; }
        }
        OrderId = fid; TotalPrice = tot; OrderPlaced = true;
        StepTitle = $"订单已生成（含 {legs.Count} 段行程）";
    }

    // ============ 乘客勾选 ============

    public void TogglePassenger(SelectablePassenger sp)
    {
        sp.IsSelected = !sp.IsSelected;
        OnPropertyChanged(nameof(SelectedCount));
        OnPropertyChanged(nameof(TotalPriceDisplay));
        OnPropertyChanged(nameof(PricePerPerson));
        PlaceOrderCommand.NotifyCanExecuteChanged();
    }

    // ============ 支付 ============

    [RelayCommand]
    private async Task PayAsync()
    {
        if (OrderId == null) return;
        ErrorMessage = string.Empty; IsLoading = true;
        try
        {
            var r = await HttpLogger.PostAsync(_httpClient, $"/api/orders/{OrderId}/pay");
            if (r.IsSuccessStatusCode) { PaymentDone = true; StepTitle = "支付成功"; }
            else { var e = await r.Content.ReadFromJsonAsync<ApiResponse<object>>(); ErrorMessage = e?.Message ?? "支付失败"; }
        }
        catch (HttpRequestException ex) { Logger.Error($"支付网络异常 [{OrderId}]: {ex.Message}"); ErrorMessage = "无法连接服务器"; }
        catch (Exception ex) { Logger.Error($"支付异常 [{OrderId}]: {ex}"); ErrorMessage = $"支付失败: {ex.Message}"; }
        finally { IsLoading = false; }
    }

    [RelayCommand] private void Close(Window? w) { if (PaymentDone) w!.DialogResult = true; w?.Close(); }
    [RelayCommand] private void Cancel(Window? w) => w?.Close();
}

// ============ 支持类 ============

/// <summary>可选乘客（带勾选状态）</summary>
public partial class SelectablePassenger : ObservableObject
{
    public SavedPassenger Passenger { get; set; } = null!;
    [ObservableProperty] private bool _isSelected;
}

/// <summary>中转购票时, 每段行程的选座包装</summary>
public partial class TransferLegOption : ObservableObject
{
    public TransferLeg Leg { get; set; } = null!;
    public ObservableCollection<SeatTypeInfo> SeatTypes { get; } = new();
    [ObservableProperty] private SeatTypeInfo? _selectedSeatType;
}
