using System.Collections.Generic;

namespace TTPFrontEnd.Models;

/// <summary>
/// 车次信息
/// </summary>
public class TrainInfo
{
    public string TrainNo { get; set; } = string.Empty;
    public string FromStation { get; set; } = string.Empty;
    public string ToStation { get; set; } = string.Empty;
    public string DepartureTime { get; set; } = string.Empty;
    public string ArrivalTime { get; set; } = string.Empty;
    public string Duration { get; set; } = string.Empty;
    public string Date { get; set; } = string.Empty;
    /// <summary>全程总距离（公里），由 API 根据经停站距离计算返回</summary>
    public int TotalDistanceKm { get; set; }
    public List<SeatTypeInfo> SeatTypes { get; set; } = new();
}

/// <summary>
/// 座位类型及余票。Price 字段在此系统中表示每公里单价（元/公里）。
/// </summary>
public class SeatTypeInfo
{
    public string Type { get; set; } = string.Empty;
    /// <summary>每公里单价（元/公里）</summary>
    public double Price { get; set; }
    public int Remain { get; set; }
}

/// <summary>
/// 经停站
/// </summary>
public class TrainStop
{
    public string Station { get; set; } = string.Empty;
    public string Arrive { get; set; } = string.Empty;
    public string Depart { get; set; } = string.Empty;
    public int Seq { get; set; }
    /// <summary>上一站到本站的距离（公里）</summary>
    public int DistanceKm { get; set; }
}

/// <summary>
/// 车次详情（含经停站）
/// </summary>
public class TrainDetailInfo : TrainInfo
{
    public List<TrainStop> Stops { get; set; } = new();
}
