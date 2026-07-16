# 火车票务系统前端对接说明

## 目标

完成本说明中的前端功能，并与后端新增接口对接后，项目可覆盖第五题的全部要求：历史订单统计、动态票价、并发订单处理、图结构存储线路以及 Dijkstra/Floyd 最短路径换乘。

现有车次查询、订单列表、购票、退款、改签和管理员后台均保留，在原有页面上增量开发即可。

## 统一约定

所有接口沿用既有响应结构：

```json
{
  "code": 0,
  "message": "ok",
  "data": {}
}
```

| code | 含义 | 前端处理 |
| --- | --- | --- |
| `0` | 成功 | 更新页面数据 |
| `40401` | 资源不存在 | 显示接口返回的 `message` |
| `40901` | 余票不足或并发购票失败 | 提示“余票或票价已发生变化，请重新查询”，刷新当前查询结果 |

继续使用现有登录 Token 和 `HttpLogger` 请求封装。

## 1. 历史订单统计（已有订单列表，补强统计展示）

现有“我的订单”已完成历史订单查询，并支持按订单状态筛选；这部分不需要重做。

为在答辩中明确对应“历史订单统计”要求，只需在原订单列表基础上补充下面的聚合统计和日期范围筛选。

### 页面改动

在“我的订单”页顶部、订单状态筛选控件上方增加统计区。建议使用 4 到 6 个统计卡片：

- 总订单数
- 已支付订单数
- 已退款订单数
- 已改签订单数
- 累计消费金额
- 退款金额

增加开始日期、结束日期和查询按钮。原有订单状态筛选与订单列表保留。

### 接口

```http
GET /api/orders/statistics?startDate=2026-07-01&endDate=2026-07-31
```

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "totalOrders": 12,
    "paidOrders": 8,
    "refundedOrders": 2,
    "changedOrders": 1,
    "totalSpent": 2486.0,
    "refundedAmount": 342.0
  }
}
```

### 模型

```csharp
public class OrderStatistics
{
    public int TotalOrders { get; set; }
    public int PaidOrders { get; set; }
    public int RefundedOrders { get; set; }
    public int ChangedOrders { get; set; }
    public double TotalSpent { get; set; }
    public double RefundedAmount { get; set; }
}
```

## 2. 动态票价（保持现有“单位票价 × 里程”）

### 已有前端逻辑

直达购票链路已经使用单位票价计算总价：

```text
总价 = SeatTypeInfo.Price × TotalDistanceKm
```

因此 `SeatTypeInfo.Price` 的语义必须保持为**当前单位票价（元/km）**，不能改成总票价。

后端会按余票等规则直接返回动态后的 `price`（仍是元/km）。直达车次查询、排序、购票窗口都可以继续使用现有计算逻辑，**不需要增加 `basePrice` 或 `currentPrice` 字段，也不需要修改直达票价公式**。

### 前端唯一需要补强的位置：中转购票

当前中转逻辑会忽略后端返回的席别数据，使用前端写死的 `SeatRate` 生成席别与价格。这会绕过后端动态票价，必须调整。

后端的 `/api/tickets/transfer` 和 `/api/routes/shortest` 会为每一段 `leg` 返回实际可选席别 `seatOptions`。前端直接反序列化到 `TransferLeg.SeatOptions`，不再调用 `SeatRate.GenerateOptions(leg)`。

每个中转 `seatOptions` 中的 `price` 同样是当前单位票价（元/km）；中转每段预估价由前端按 `seat.Price × leg.DistanceKm` 计算，总价为各段相加。

```json
{
  "type": "二等座",
  "price": 0.45,
  "remain": 6,
  "adjustmentReason": "余票不足 20%，加价 8%"
}
```

`adjustmentReason` 是可选展示字段，不影响现有计算。

### 下单规则

前端计算的总价只用于预览。订单提交成功后，必须以创建订单接口返回的 `totalPrice` 为最终成交价。
## 3. 最短路径与智能换乘

### 页面改动

在现有“车次查询”页增加两个下拉框：

| 控件 | 选项 |
| --- | --- |
| 路线策略 | 最短时间、最低票价、最短距离 |
| 计算算法 | Dijkstra、Floyd |

规则：Dijkstra 支持三种策略；Floyd 仅支持最短距离。若用户选择 Floyd 后又选择最低票价或最短时间，禁用查询或提示切换到 Dijkstra。

### 接口

```http
GET /api/routes/shortest?from=BJP&to=GZQ&date=2026-07-20&strategy=time&algorithm=dijkstra
```

| 参数 | 说明 |
| --- | --- |
| `from` | 出发站编码 |
| `to` | 到达站编码 |
| `date` | 出发日期 |
| `strategy` | `time` / `price` / `distance` |
| `algorithm` | `dijkstra` / `floyd` |

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "algorithm": "dijkstra",
    "strategy": "time",
    "transferCount": 1,
    "totalDistanceKm": 2150,
    "totalDurationMinutes": 810,
    "totalPrice": 932.0,
    "legs": [
      {
        "trainNo": "G100",
        "fromStation": "北京",
        "toStation": "郑州",
        "departureTime": "06:40",
        "arrivalTime": "09:18",
        "duration": "2h38min",
        "distanceKm": 693,
        "price": 286.0,
        "seatOptions": []
      },
      {
        "trainNo": "G306",
        "fromStation": "郑州",
        "toStation": "广州",
        "departureTime": "10:00",
        "arrivalTime": "18:20",
        "duration": "8h20min",
        "distanceKm": 1457,
        "price": 646.0,
        "seatOptions": []
      }
    ]
  }
}
```

### 展示要求

复用现有中转路线卡片样式，并增加：

```text
Dijkstra · 最短时间
换乘 1 次 · 总距离 2150 km · 总耗时 13h30min · 总价 ¥932
```

多段路线仍可点击“购买”，并复用现有中转购票窗口；每一段可分别选择席别。

### 模型

```csharp
public class ShortestRouteResult
{
    public string Algorithm { get; set; } = string.Empty;
    public string Strategy { get; set; } = string.Empty;
    public int TransferCount { get; set; }
    public int TotalDistanceKm { get; set; }
    public int TotalDurationMinutes { get; set; }
    public double TotalPrice { get; set; }
    public List<TransferLeg> Legs { get; set; } = new();
}
```

## 4. 线路图展示

在管理员后台增加“线路图”按钮或页签。

最小可行版本无需绘制复杂图形，展示站点及相邻线路即可：

```text
北京
  → 南京 1000 km
  → 郑州 693 km
```

若时间充足，可再使用 Canvas 绘制站点节点和边。

### 接口

```http
GET /api/network/graph
```

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "stations": [
      {
        "code": "BJP",
        "name": "北京",
        "city": "北京",
        "edges": [
          {
            "toCode": "NJH",
            "toName": "南京",
            "distance": 1000,
            "trainNos": ["G100", "G101"]
          }
        ]
      }
    ]
  }
}
```

现有 `StationInfo.Edges` 可直接复用，在边模型中增加：

```csharp
public string ToName { get; set; } = string.Empty;
public List<string> TrainNos { get; set; } = new();
```

## 5. 并发购票测试

在管理员后台增加“并发购票测试”区域，仅管理员可见。

输入项：车次、席别、日期、模拟请求数。

结果展示：

```text
请求数：10
成功：3
失败：7
当前剩余票数：0
```

### 接口

```http
POST /api/admin/concurrency-test
Content-Type: application/json
```

请求：

```json
{
  "trainNo": "G100",
  "seatType": "二等座",
  "date": "2026-07-20",
  "requestCount": 10
}
```

响应：

```json
{
  "code": 0,
  "message": "concurrency test complete",
  "data": {
    "requestedCount": 10,
    "successCount": 3,
    "failedCount": 7,
    "remainingSeats": 0
  }
}
```

```csharp
public class ConcurrencyTestResult
{
    public int RequestedCount { get; set; }
    public int SuccessCount { get; set; }
    public int FailedCount { get; set; }
    public int RemainingSeats { get; set; }
}
```

此功能仅用于答辩演示后端锁、余票扣减和防超卖机制，普通用户购票页面无需增加。

## 前端改动清单

- 保留已有订单列表和状态筛选；在订单页补充统计卡片、日期范围筛选和统计接口调用。
- 在 `SeatTypeInfo` 中增加动态价格字段，并在查询、购票、改签页面展示。
- 在车次查询页增加路线策略和算法选择，调用最短路径接口。
- 将最短路径结果转换为现有路线卡片与中转购票数据。
- 在管理员后台增加线路图入口和并发测试入口。
- 购票、退款、改签成功后，刷新订单列表、统计卡片和车次余票。
- 处理 `40901`：提示用户重新查询，禁止继续用旧价格提交订单。

## 验收演示流程

1. 管理员新增站点和车次，展示线路图中的站点和边。
2. 用户查询两站路线，分别展示最短时间、最低票价、最短距离。
3. 展示 Dijkstra 与 Floyd 的路径计算结果。
4. 车次列表展示余票和动态票价。
5. 并发测试模拟多人购买最后几张票，证明不会超卖。
6. 退款后余票恢复、动态票价变化。
7. 改签后原席别余票恢复、新席别余票扣减。
8. 在“我的订单”展示历史订单、筛选和消费统计。

后端将严格按照本说明的接口实现，前端可据此开始开发。
