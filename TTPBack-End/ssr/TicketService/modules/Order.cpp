// 文件说明：订单模块：负责创建订单、支付订单和查询订单。

#include "../TicketService.h"

#include <algorithm>
#include <iomanip>
#include <mutex>
#include <sstream>

std::string TicketService::CreateOrderJson(const std::string& requestBody, const std::string& usernameOverride) {
    std::lock_guard<std::mutex> lock(mutex_);

    Order baseOrder;
    baseOrder.username = usernameOverride.empty() ? ExtractString(requestBody, "username") : usernameOverride;
    if (baseOrder.username.empty())
        baseOrder.username = profile_.username.empty() ? "user" : profile_.username;
    baseOrder.trainNo = ExtractString(requestBody, "trainNo");
    baseOrder.date = ExtractString(requestBody, "date");
    baseOrder.fromStation = ResolveStationName(DecodeLooseUnicodeEscapes(ExtractString(requestBody, "fromStation")));
    baseOrder.toStation = ResolveStationName(DecodeLooseUnicodeEscapes(ExtractString(requestBody, "toStation")));
    baseOrder.seatType = DecodeLooseUnicodeEscapes(ExtractString(requestBody, "seatType"));
    baseOrder.seatNo = "05A";
    baseOrder.status = "unpaid";
    baseOrder.createdAt = "2026-07-06T14:30:00";
    baseOrder.expireAt = "2026-07-06T14:45:00";

    auto passengers = ExtractPassengersFromOrderJson(requestBody);
    if (passengers.empty())
        passengers.push_back({DecodeLooseUnicodeEscapes(ExtractString(requestBody, "realName")), ExtractString(requestBody, "idCard")});

    Train* train = FindTrain(baseOrder.trainNo);
    if (train == nullptr)
        return ApiResponse("null", 40401, "train not found");

    SegmentInfo segment;
    if (!TryGetSegment(*train, baseOrder.fromStation, baseOrder.toStation, segment))
        return ApiResponse("null", 40001, "invalid train segment");

    // 余票按车次和席别维护；一次为多名乘客下单时必须一次性预留全部席位。
    auto seatIt = std::find_if(train->seatTypes.begin(), train->seatTypes.end(), [&](const SeatType& seat) {
        return seat.type == baseOrder.seatType;
    });
    if (seatIt == train->seatTypes.end())
        return ApiResponse("null", 40402, "seat type not found");
    if (seatIt->remain < static_cast<int>(passengers.size()))
        return ApiResponse("null", 40002, "not enough seats");

    const double singleTicketPrice = TripPrice(*seatIt, segment.distanceKm);
    // 只有全部校验通过后才扣减，防止创建失败的订单污染库存。
    seatIt->remain -= static_cast<int>(passengers.size());

    Order responseOrder = baseOrder;
    responseOrder.totalPrice = singleTicketPrice * static_cast<double>(passengers.size());
    for (std::size_t i = 0; i < passengers.size(); ++i) {
        Order order = baseOrder;
        order.orderId = "20260710" + std::to_string(nextOrderId_++);
        order.realName = passengers[i].first;
        order.idCard = passengers[i].second;
        order.seatNo = "05" + std::string(1, static_cast<char>('A' + static_cast<int>(i % 6)));
        order.totalPrice = singleTicketPrice;
        if (i == 0) {
            responseOrder = order;
            responseOrder.totalPrice = singleTicketPrice * static_cast<double>(passengers.size());
        }
        orders_.push_back(order);
    }
    // trains.csv 与 orders.csv 一并保存，服务重启后余票和订单仍保持一致。
    SaveData();

    return ApiResponse(OrderToJson(responseOrder), 0, "order created");
}std::string TicketService::PayOrderJson(const std::string& orderId) {
    // 将待支付订单改为已支付状态。
    std::lock_guard<std::mutex> lock(mutex_);
    Order* order = FindOrder(orderId);
    // 先确认目标数据存在，避免访问无效内容。
    if (order == nullptr)
        return ApiResponse("null", 40401, "order not found");
    order->status = "paid";
    int paidCount = 1;
    // 同一行程或可衔接行程的未支付订单会一起更新为已支付。
    bool changed = true;
    // 持续处理，直到满足结束条件。
    while (changed) {
        changed = false;
        // 依次处理集合中的每一项数据。
        for (auto& candidate : orders_) {
            // 根据订单当前状态决定是否继续处理。
            if (candidate.status == "paid")
                continue;
            // 根据当前条件决定是否进入下一步处理。
            if (candidate.username != order->username || candidate.date != order->date)
                continue;
            const bool sameTrip = candidate.trainNo == order->trainNo && candidate.fromStation == order->fromStation && candidate.toStation == order->toStation && candidate.seatType == order->seatType;
            // 根据当前条件决定是否进入下一步处理。
            if (sameTrip) {
                candidate.status = "paid";
                ++paidCount;
                changed = true;
                continue;
            }
            // 根据当前条件决定是否进入下一步处理。
            if (!sameTrip) {

            // 依次处理集合中的每一项数据。
            for (const auto& paidOrder : orders_) {
                // 根据订单当前状态决定是否继续处理。
                if (paidOrder.status != "paid")
                    continue;
                // 根据订单当前状态决定是否继续处理。
                if (paidOrder.username != order->username || paidOrder.date != order->date)
                    continue;
                const bool connectedForward = paidOrder.toStation == candidate.fromStation;
                const bool connectedBackward = candidate.toStation == paidOrder.fromStation;
                // 根据当前条件决定是否进入下一步处理。
                if (connectedForward || connectedBackward) {
                    candidate.status = "paid";
                    ++paidCount;
                    changed = true;
                    break;
                }
            }
            }
        }
    }
    SaveData();
    return ApiResponse("{\"orderId\":\"" + EscapeJson(orderId) + "\",\"status\":\"paid\",\"paidCount\":" + std::to_string(paidCount) + "}", 0, "pay success");
}
std::string TicketService::GetOrdersJson(const std::string& username, const std::string& status, const std::string& date) const {
    // 读取所需数据，并整理为接口需要的返回结果。
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string effectiveUsername = username.empty() ? profile_.username : username;
    // 先确认目标数据存在，避免访问无效内容。
    if (effectiveUsername.empty())
        return ApiResponse("[]", 0, "ok");
    std::ostringstream data;
    data << '[';
    bool first = true;
    // 逐条检查订单是否满足用户、状态和日期筛选条件。
    for (const auto& order : orders_) {
        // 根据当前条件决定是否进入下一步处理。
        if (order.username != effectiveUsername)
            continue;
        // 先确认目标数据存在，避免访问无效内容。
        if (!status.empty() && order.status != status)
            continue;
        // 先确认目标数据存在，避免访问无效内容。
        if (!date.empty() && order.date != date)
            continue;
        // 根据当前条件决定是否进入下一步处理。
        if (!first)
            data << ',';
        data << OrderToJson(order, true);
        first = false;
    }
    data << ']';
    return ApiResponse(data.str(), 0, "ok");
}
TicketService::Order* TicketService::FindOrder(const std::string& orderId) {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    const auto it = std::find_if(orders_.begin(), orders_.end(), [&](const Order& order) { return order.orderId == orderId; });
    return it == orders_.end() ? nullptr : &(*it);
}

