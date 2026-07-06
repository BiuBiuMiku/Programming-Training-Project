# 铁路票务系统 API 接口规范

## 约定

- 基础路径：`http://localhost:8080`
- Content-Type：`application/json`
- 所有响应统一包层：`{ "code": 0, "message": "xxx", "data": {...} }`
- code 为 0 表示成功，非 0 一律视为失败，前端取 message 字段展示错误
- 需要认证的接口在 Header 中携带：`Authorization: Bearer <token>`

---

## 通用错误码

| 错误码 | 说明 |
|--------|------|
| 40001 | 参数校验失败（字段缺失/格式错误） |
| 40100 | 未登录 / Token 过期 |
| 40101 | 用户名或密码错误 |
| 40102 | 账号已被禁用 |
| 40300 | 权限不足（普通用户访问管理员接口） |
| 40401 | 资源不存在 |
| 40900 | 冲突（如余票不足、重复下单） |
| 50000 | 服务器内部错误 |

---

## M1 · 用户登录与权限管理

### 用户注册

**方法/路径：** `POST /api/auth/register`
**认证：** 否

**请求：**

```json
{
    "username": "zhangsan",
    "password": "123456",
    "realName": "张三",
    "idCard": "110101199001011234",
    "phone": "13800138000"
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| username | string | 是 | 用户名，3-20 位字母数字下划线 |
| password | string | 是 | 密码，6-30 位 |
| realName | string | 是 | 真实姓名 |
| idCard | string | 是 | 18 位身份证号 |
| phone | string | 是 | 11 位手机号 |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "注册成功",
    "data": null
}
```

---

### 用户登录

**方法/路径：** `POST /api/auth/login`
**认证：** 否

**请求：**

```json
{
    "username": "zhangsan",
    "password": "123456"
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| username | string | 是 | 用户名 |
| password | string | 是 | 密码 |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "登录成功",
    "data": {
        "token": "eyJhbGciOiJIUzI1NiIs...",
        "username": "zhangsan",
        "role": "user"
    }
}
```

| data 字段 | 类型 | 说明 |
|-----------|------|------|
| token | string | JWT Token，后续请求携带 |
| username | string | 用户名 |
| role | string | 角色：user / admin |

**失败示例：**

```json
{
    "code": 40101,
    "message": "用户名或密码错误",
    "data": null
}
```

---

### 获取当前用户信息

**方法/路径：** `GET /api/auth/me`
**认证：** 是

**请求：** 无 Body

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "ok",
    "data": {
        "id": 1,
        "username": "zhangsan",
        "realName": "张三",
        "idCard": "110101********1234",
        "phone": "138****8000",
        "role": "user"
    }
}
```

> idCard 和 phone 做脱敏处理后返回。

---

## M2 · 车次与线路管理

### 站点列表

**方法/路径：** `GET /api/stations`
**认证：** 否

**请求：** 无 Body

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "ok",
    "data": [
        { "code": "BJP", "name": "北京", "city": "北京" },
        { "code": "SHH", "name": "上海", "city": "上海" },
        { "code": "GZQ", "name": "广州", "city": "广州" }
    ]
}
```

| data[n] 字段 | 类型 | 说明 |
|--------------|------|------|
| code | string | 站名拼音码（电报码） |
| name | string | 站名 |
| city | string | 所在城市 |

---

### 车次查询

**方法/路径：** `GET /api/trains`
**认证：** 否

**请求：** Query 参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| from | string | 否 | 出发站 code |
| to | string | 否 | 到达站 code |
| date | string | 否 | 日期，格式 yyyy-MM-dd |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "ok",
    "data": [
        {
            "trainNo": "G101",
            "fromStation": "北京",
            "toStation": "上海",
            "departureTime": "08:00",
            "arrivalTime": "12:30",
            "duration": "4h30min",
            "date": "2026-07-10",
            "seatTypes": [
                { "type": "二等座", "price": 553.0, "remain": 120 },
                { "type": "一等座", "price": 933.0, "remain": 45 },
                { "type": "商务座", "price": 1748.0, "remain": 8 }
            ]
        }
    ]
}
```

| data[n] 字段 | 类型 | 说明 |
|--------------|------|------|
| trainNo | string | 车次号 |
| fromStation | string | 出发站名 |
| toStation | string | 到达站名 |
| departureTime | string | 出发时间 HH:mm |
| arrivalTime | string | 到达时间 HH:mm |
| duration | string | 运行时长 |
| date | string | 日期 yyyy-MM-dd |
| seatTypes[].type | string | 座位类型名称 |
| seatTypes[].price | double | 票价（元） |
| seatTypes[].remain | int | 余票数 |

---

### 车次详情（含经停站）

**方法/路径：** `GET /api/trains/{trainNo}`
**认证：** 否

**示例：** `GET /api/trains/G101`

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "ok",
    "data": {
        "trainNo": "G101",
        "fromStation": "北京",
        "toStation": "上海",
        "departureTime": "08:00",
        "arrivalTime": "12:30",
        "duration": "4h30min",
        "date": "2026-07-10",
        "stops": [
            { "station": "北京", "arrive": "-",    "depart": "08:00", "seq": 1 },
            { "station": "济南", "arrive": "09:30", "depart": "09:33", "seq": 2 },
            { "station": "南京", "arrive": "11:10", "depart": "11:13", "seq": 3 },
            { "station": "上海", "arrive": "12:30", "depart": "-",    "seq": 4 }
        ]
    }
}
```

| stops[n] | 类型 | 说明 |
|----------|------|------|
| station | string | 站名 |
| arrive | string | 到站时间，首站为 "-" |
| depart | string | 发车时间，末站为 "-" |
| seq | int | 站序 |

---

## M3 · 票务核心业务（含智能中转）

### 车票查询（直达）

**方法/路径：** `GET /api/tickets`
**认证：** 否

**请求：** Query 参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| from | string | 是 | 出发站 code |
| to | string | 是 | 到达站 code |
| date | string | 是 | 日期 yyyy-MM-dd |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "ok",
    "data": {
        "from": "北京",
        "to": "上海",
        "date": "2026-07-10",
        "trains": [
            {
                "trainNo": "G101",
                "fromStation": "北京",
                "toStation": "上海",
                "departureTime": "08:00",
                "arrivalTime": "12:30",
                "duration": "4h30min",
                "seatTypes": [
                    { "type": "二等座", "price": 553.0, "remain": 120 },
                    { "type": "一等座", "price": 933.0, "remain": 45 }
                ]
            }
        ]
    }
}
```

---

### 中转推荐

**方法/路径：** `GET /api/tickets/transfer`
**认证：** 否

**请求：** Query 参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| from | string | 是 | 出发站 code |
| to | string | 是 | 到达站 code |
| date | string | 是 | 日期 yyyy-MM-dd |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "ok",
    "data": {
        "directTrains": [
            { "trainNo": "G101", "departureTime": "08:00", "arrivalTime": "12:30",
              "duration": "4h30min", "minPrice": 553.0 }
        ],
        "transferRoutes": [
            {
                "totalDuration": "5h20min",
                "totalPrice": 480.0,
                "transferStation": "济南",
                "legs": [
                    {
                        "trainNo": "G101",
                        "fromStation": "北京",
                        "toStation": "济南",
                        "departureTime": "08:00",
                        "arrivalTime": "09:30",
                        "seatType": "二等座",
                        "price": 195.0
                    },
                    {
                        "trainNo": "G203",
                        "fromStation": "济南",
                        "toStation": "上海",
                        "departureTime": "10:15",
                        "arrivalTime": "13:20",
                        "seatType": "二等座",
                        "price": 285.0
                    }
                ]
            }
        ]
    }
}
```

| transferRoutes[n] | 类型 | 说明 |
|--------------------|------|------|
| totalDuration | string | 总耗时 |
| totalPrice | double | 总票价 |
| transferStation | string | 中转站 |
| legs | array | 两段行程 |

---

### 提交订单（购票下单）

**方法/路径：** `POST /api/orders`
**认证：** 是

**请求：**

```json
{
    "trainNo": "G101",
    "date": "2026-07-10",
    "fromStation": "北京",
    "toStation": "上海",
    "seatType": "二等座",
    "passengers": [
        {
            "realName": "张三",
            "idCard": "110101199001011234"
        }
    ]
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| trainNo | string | 是 | 车次号 |
| date | string | 是 | 乘车日期 yyyy-MM-dd |
| fromStation | string | 是 | 出发站名 |
| toStation | string | 是 | 到达站名 |
| seatType | string | 是 | 座位类型 |
| passengers | array | 是 | 乘客列表，最少 1 人 |
| passengers[].realName | string | 是 | 真实姓名 |
| passengers[].idCard | string | 是 | 身份证号 |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "下单成功，请在15分钟内完成支付",
    "data": {
        "orderId": "202607100001",
        "trainNo": "G101",
        "date": "2026-07-10",
        "fromStation": "北京",
        "toStation": "上海",
        "seatType": "二等座",
        "totalPrice": 553.0,
        "passengers": [
            { "realName": "张三", "idCard": "110101********1234", "seatNo": "05车12A号" }
        ],
        "status": "unpaid",
        "createdAt": "2026-07-06T14:30:00",
        "expireAt": "2026-07-06T14:45:00"
    }
}
```

| data 字段 | 类型 | 说明 |
|-----------|------|------|
| orderId | string | 订单号 |
| status | string | unpaid / paid / cancelled / refunded / changed |
| totalPrice | double | 总票价 |
| expireAt | string | 支付截止时间 ISO 8601 |

---

### 支付订单

**方法/路径：** `POST /api/orders/{orderId}/pay`
**认证：** 是

**示例：** `POST /api/orders/202607100001/pay`

**请求：** 无 Body（模拟支付，无需真实支付参数）

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "支付成功",
    "data": {
        "orderId": "202607100001",
        "status": "paid"
    }
}
```

---

### 查询我的订单

**方法/路径：** `GET /api/orders`
**认证：** 是

**请求：** Query 参数（均可选）

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| status | string | 否 | 筛选状态：unpaid / paid / refunded / changed |
| date | string | 否 | 筛选日期 yyyy-MM-dd |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "ok",
    "data": [
        {
            "orderId": "202607100001",
            "trainNo": "G101",
            "date": "2026-07-10",
            "fromStation": "北京",
            "toStation": "上海",
            "departureTime": "08:00",
            "seatType": "二等座",
            "totalPrice": 553.0,
            "status": "paid",
            "createdAt": "2026-07-06T14:30:00"
        }
    ]
}
```

---

## M4 · 退票与改签

### 查询退票手续费

**方法/路径：** `GET /api/orders/{orderId}/refund-fee`
**认证：** 是

**示例：** `GET /api/orders/202607100001/refund-fee`

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "ok",
    "data": {
        "orderId": "202607100001",
        "originalPrice": 553.0,
        "fee": 27.65,
        "refundAmount": 525.35,
        "feeRate": "5%",
        "reason": "距离开车前48小时以上，按票价5%收取退票费"
    }
}
```

| data 字段 | 类型 | 说明 |
|-----------|------|------|
| originalPrice | double | 原票价 |
| fee | double | 手续费 |
| refundAmount | double | 应退金额 |
| feeRate | string | 手续费率 |
| reason | string | 费率说明 |

---

### 申请退票

**方法/路径：** `POST /api/orders/{orderId}/refund`
**认证：** 是

**示例：** `POST /api/orders/202607100001/refund`

**请求：**

```json
{
    "reason": "行程变更"
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| reason | string | 否 | 退票原因 |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "退票成功",
    "data": {
        "orderId": "202607100001",
        "status": "refunded",
        "refundAmount": 525.35,
        "fee": 27.65,
        "refundedAt": "2026-07-06T15:00:00"
    }
}
```

**失败示例：**

```json
{
    "code": 40900,
    "message": "列车已发车，无法退票",
    "data": null
}
```

---

### 查询改签可选车次

**方法/路径：** `GET /api/orders/{orderId}/change-options`
**认证：** 是

**请求：** Query 参数

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| date | string | 否 | 目标日期 yyyy-MM-dd，默认当天 |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "ok",
    "data": {
        "originalOrder": {
            "orderId": "202607100001",
            "trainNo": "G101",
            "fromStation": "北京",
            "toStation": "上海",
            "date": "2026-07-10",
            "seatType": "二等座",
            "price": 553.0
        },
        "availableTrains": [
            {
                "trainNo": "G103",
                "fromStation": "北京",
                "toStation": "上海",
                "departureTime": "10:00",
                "arrivalTime": "14:30",
                "seatTypes": [
                    { "type": "二等座", "price": 553.0, "remain": 35 },
                    { "type": "一等座", "price": 933.0, "remain": 12 }
                ],
                "priceDiff": 0.0
            }
        ]
    }
}
```

| availableTrains[n] | 类型 | 说明 |
|---------------------|------|------|
| priceDiff | double | 与原始订单的差价（正数需补，负数退还） |

---

### 申请改签

**方法/路径：** `POST /api/orders/{orderId}/change`
**认证：** 是

**示例：** `POST /api/orders/202607100001/change`

**请求：**

```json
{
    "newTrainNo": "G103",
    "newDate": "2026-07-10",
    "newSeatType": "一等座"
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| newTrainNo | string | 是 | 新车次号 |
| newDate | string | 是 | 新日期 yyyy-MM-dd |
| newSeatType | string | 是 | 新座位类型 |

**成功响应（200）：**

```json
{
    "code": 0,
    "message": "改签成功",
    "data": {
        "orderId": "202607100001",
        "oldTrainNo": "G101",
        "newTrainNo": "G103",
        "priceDiff": 380.0,
        "status": "changed",
        "changedAt": "2026-07-06T16:00:00"
    }
}
```

---

## 附录：JWT Token 格式约定

Token 采用 JWT HS256 签名，Payload 包含：

```json
{
    "sub": "1",
    "username": "zhangsan",
    "role": "user",
    "iat": 1750000000,
    "exp": 1750086400
}
```

| 字段 | 说明 |
|------|------|
| sub | 用户 ID |
| username | 用户名 |
| role | 角色：user / admin |
| iat | 签发时间戳 |
| exp | 过期时间戳（建议 24 小时后） |

---

> **版本：** v1.0
> **最后更新：** 2026-07-06
> **前后端确认人签字：** ____________
