using System;
using System.Collections.Generic;
using TTPFrontEnd.Utils;

namespace TTPFrontEnd.Models;

/// <summary>
/// 中转推荐查询结果
/// </summary>
public class TransferResult
{
    public List<TrainInfo> DirectTrains { get; set; } = new();
    public List<TransferRoute> TransferRoutes { get; set; } = new();
}

/// <summary>
/// 一条中转路线
/// </summary>
public class TransferRoute
{
    public string TotalDuration { get; set; } = string.Empty;
    public double TotalPrice { get; set; }
    public string TransferStation { get; set; } = string.Empty;
    public List<TransferLeg> Legs { get; set; } = new();
}

/// <summary>
/// 中转的一段行程
/// </summary>
public class TransferLeg
{
    public string TrainNo { get; set; } = string.Empty;
    public string FromStation { get; set; } = string.Empty;
    public string ToStation { get; set; } = string.Empty;
    public string DepartureTime { get; set; } = string.Empty;
    public string ArrivalTime { get; set; } = string.Empty;
    /// <summary>该段距离（公里）</summary>
    public int DistanceKm { get; set; }
    /// <summary>该段可选的座位类型列表（API 直接返回，含该段准确总价）</summary>
    public List<SeatTypeInfo> SeatTypes { get; set; } = new();
    public string SeatType { get; set; } = string.Empty;
    public double Price { get; set; }
}

/// <summary>
/// 购票提交请求
/// </summary>
public class OrderRequest
{
    public string TrainNo { get; set; } = string.Empty;
    public string Date { get; set; } = string.Empty;
    public string FromStation { get; set; } = string.Empty;
    public string ToStation { get; set; } = string.Empty;
    public string SeatType { get; set; } = string.Empty;
    public List<PassengerInfo> Passengers { get; set; } = new();
}

/// <summary>
/// 乘客
/// </summary>
public class PassengerInfo
{
    public string RealName { get; set; } = string.Empty;
    public string IdCard { get; set; } = string.Empty;
}

/// <summary>
/// 订单信息（下单返回 + 列表展示）
/// </summary>
public class OrderInfo
{
    public string OrderId { get; set; } = string.Empty;
    public string TrainNo { get; set; } = string.Empty;
    public string Date { get; set; } = string.Empty;
    public string FromStation { get; set; } = string.Empty;
    public string ToStation { get; set; } = string.Empty;
    public string DepartureTime { get; set; } = string.Empty;
    public string SeatType { get; set; } = string.Empty;
    public double TotalPrice { get; set; }
    public List<PassengerSeat> Passengers { get; set; } = new();
    public string Status { get; set; } = string.Empty;
    public string CreatedAt { get; set; } = string.Empty;
    public string ExpireAt { get; set; } = string.Empty;
}

public class PassengerSeat
{
    public string RealName { get; set; } = string.Empty;
    public string IdCard { get; set; } = string.Empty;
    public string SeatNo { get; set; } = string.Empty;
}

/// <summary>
/// 退票费率查询结果
/// </summary>
public class RefundFeeInfo
{
    public string OrderId { get; set; } = string.Empty;
    public double OriginalPrice { get; set; }
    public double Fee { get; set; }
    public double RefundAmount { get; set; }
    public string FeeRate { get; set; } = string.Empty;
    public string Reason { get; set; } = string.Empty;
}

/// <summary>
/// 改签请求
/// </summary>
public class ChangeRequest
{
    public string NewTrainNo { get; set; } = string.Empty;
    public string NewDate { get; set; } = string.Empty;
    public string NewSeatType { get; set; } = string.Empty;
}

/// <summary>
/// 改签可选车次结果
/// </summary>
public class ChangeOptionsResult
{
    public OrderInfo? OriginalOrder { get; set; }
    public List<ChangeTrainOption> AvailableTrains { get; set; } = new();
}

public class ChangeTrainOption
{
    public string TrainNo { get; set; } = string.Empty;
    public string FromStation { get; set; } = string.Empty;
    public string ToStation { get; set; } = string.Empty;
    public string DepartureTime { get; set; } = string.Empty;
    public string ArrivalTime { get; set; } = string.Empty;
    public List<SeatTypeInfo> SeatTypes { get; set; } = new();
    public double PriceDiff { get; set; }
}

/// <summary>
/// 用户个人信息
/// </summary>
public class UserProfile
{
    public string RealName { get; set; } = string.Empty;
    public string IdCard { get; set; } = string.Empty;
    public string Phone { get; set; } = string.Empty;
    public string Username { get; set; } = string.Empty;
    public string Role { get; set; } = string.Empty;
}

/// <summary>
/// 更新个人信息的请求
/// </summary>
public class UpdateProfileRequest
{
    public string RealName { get; set; } = string.Empty;
    public string IdCard { get; set; } = string.Empty;
    public string Phone { get; set; } = string.Empty;
}

/// <summary>
/// 状态中文映射
/// </summary>
public static class OrderStatusText
{
    public static string Get(string status) => status switch
    {
        "unpaid" => "待支付",
        "paid" => "已支付",
        "refunded" => "已退票",
        "changed" => "已改签",
        "cancelled" => "已取消",
        _ => status
    };
}

/// <summary>
/// 已保存的乘客身份（本地存储用）
/// </summary>
public class SavedPassenger
{
    public string Id { get; set; } = Guid.NewGuid().ToString("N")[..8];
    public string RealName { get; set; } = string.Empty;
    public string IdCard { get; set; } = string.Empty;
    public string Phone { get; set; } = string.Empty;
}

/// <summary>
/// 乘客身份本地存储，按用户名隔离
/// </summary>
public static class PassengerStorage
{
    private static string GetPath(string username)
    {
        var safe = string.Join("_", username.Split(System.IO.Path.GetInvalidFileNameChars()));
        return System.IO.Path.Combine(
            System.Environment.GetFolderPath(System.Environment.SpecialFolder.LocalApplicationData),
            "TTPFrontEnd", "passengers", $"passengers_{safe}.json");
    }

    public static List<SavedPassenger> Load(string username)
    {
        try
        {
            var path = GetPath(username);
            if (System.IO.File.Exists(path))
            {
                var json = System.IO.File.ReadAllText(path);
                return System.Text.Json.JsonSerializer.Deserialize<List<SavedPassenger>>(json) ?? new();
            }
        }
        catch (Exception ex) { Utils.Logger.Error($"加载乘客数据失败 [{username}]: {ex.Message}"); }
        return new();
    }

    public static void Save(string username, List<SavedPassenger> list)
    {
        try
        {
            var path = GetPath(username);
            var dir = System.IO.Path.GetDirectoryName(path);
            if (!System.IO.Directory.Exists(dir))
                System.IO.Directory.CreateDirectory(dir!);
            var json = System.Text.Json.JsonSerializer.Serialize(list);
            System.IO.File.WriteAllText(path, json);
        }
        catch (Exception ex) { Utils.Logger.Error($"保存乘客数据失败 [{username}]: {ex.Message}"); }
    }
}

/// <summary>
/// 统一的出行方案（直达或中转都包装为此类型，用于排序和统一展示）
/// </summary>
public class RouteOption
{
    /// <summary>"直达" 或 "中转"</summary>
    public string Type { get; set; } = string.Empty;
    /// <summary>直达时 = 车次号；中转时 = "G101 → G203"</summary>
    public string DisplayTrainNo { get; set; } = string.Empty;
    public string FromStation { get; set; } = string.Empty;
    public string ToStation { get; set; } = string.Empty;
    /// <summary>出发时间 HH:mm</summary>
    public string DepartureTime { get; set; } = string.Empty;
    /// <summary>到达时间 HH:mm</summary>
    public string ArrivalTime { get; set; } = string.Empty;
    public string Duration { get; set; } = string.Empty;
    /// <summary>最小票价（用于排序）</summary>
    public double MinPrice { get; set; }
    /// <summary>是否中转</summary>
    public bool IsTransfer { get; set; }
    public string? TransferStation { get; set; }
    public double TotalPrice { get; set; }
    public List<TransferLeg>? Legs { get; set; }
    /// <summary>直达车次的座位类型列表（仅直达有效）</summary>
    public List<SeatTypeInfo>? SeatTypes { get; set; }
    /// <summary>全程总距离（公里），用于计算票价</summary>
    public int TotalDistanceKm { get; set; }
    /// <summary>原始 TrainInfo（用于购票）</summary>
    public TrainInfo? SourceTrain { get; set; }
    /// <summary>原始 TransferRoute（用于中转购票）</summary>
    public TransferRoute? TransferRouteData { get; set; }
}
