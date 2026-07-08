using System.Collections.Generic;

namespace TTPFrontEnd.Models;

/// <summary>
/// 站点信息
/// </summary>
public class StationInfo
{
    public string Code { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string City { get; set; } = string.Empty;
    /// <summary>
    /// 站点与其他站的距离边，传给后端用于建图
    /// </summary>
    public List<StationDistance> Edges { get; set; } = new();

    public override string ToString() => $"{Name} ({Code})";
}

/// <summary>
/// 站点间距离（边）
/// </summary>
public class StationDistance
{
    public string FromCode { get; set; } = string.Empty;
    public string ToCode { get; set; } = string.Empty;
    public int Distance { get; set; }
}
