## M1 · 用户登录与权限管理

- [ ] `POST /api/auth/send-code` — 发送短信验证码
- [ ] `POST /api/auth/register`  — 用户注册（手机号）
- [ ] `POST /api/auth/login`     — 用户登录
- [ ] `GET  /api/auth/me`        — 获取当前用户信息（前端未接入）
- [ ] `PUT  /api/auth/profile`   — 更新个人信息（前端未接入）

## M2 · 车次与线路管理

- [ ] `GET  /api/stations`          — 站点列表
- [ ] `GET  /api/trains`            — 车次查询（按 from/to/date 筛选）
- [ ] `GET  /api/trains/{trainNo}`  — 车次详情（含经停站）

## M3 · 票务核心（含智能中转）

- [ ] `GET  /api/tickets`                     — 车票查询（直达，前端目前用 /api/trains 代替）
- [ ] `GET  /api/tickets/transfer`            — 中转推荐（结果异常，所有查询返回同一换乘方案）
- [ ] `POST /api/orders`                      — 提交订单（购票下单）
- [ ] `POST /api/orders/{orderId}/pay`        — 支付订单
- [ ] `GET  /api/orders`                      — 查询我的订单（按 status/date 筛选）

## M4 · 退票与改签

- [ ] `GET  /api/orders/{orderId}/refund-fee`     — 查询退票手续费
- [ ] `POST /api/orders/{orderId}/refund`         — 申请退票
- [ ] `GET  /api/orders/{orderId}/change-options` — 查询改签可选车次
- [ ] `POST /api/orders/{orderId}/change`         — 申请改签

## M5 · 管理员后台（需 admin 角色）

- [ ] `GET    /api/admin/trains`          — 获取所有车次
- [ ] `POST   /api/admin/trains`          — 新增车次
- [ ] `PUT    /api/admin/trains/{trainNo}` — 编辑车次
- [ ] `DELETE /api/admin/trains/{trainNo}` — 删除车次
- [ ] `POST   /api/admin/stations`        — 新增站点
- [ ] `PUT    /api/admin/stations/{code}`  — 编辑站点
- [ ] `DELETE /api/admin/stations/{code}`  — 删除站点