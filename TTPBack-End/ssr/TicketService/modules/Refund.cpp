// 文件说明：售后模块：负责退款、改签和改签候选车次查询。

#include "../TicketService.h"

#include <mutex>
#include <iomanip>
#include <sstream>

std::string TicketService::GetRefundFeeJson(const std::string& orderId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto orderIt = std::find_if(orders_.begin(), orders_.end(), [&](const Order& item) {
        return item.orderId == orderId;
    });
    if (orderIt == orders_.end())
        return ApiResponse("null", 40401, "order not found");
    const Order& order = *orderIt;
    const double fee = order.totalPrice * 0.05;

    // 退款预览必须读取订单实际成交价，不能使用固定的模拟金额。
    const double refundAmount = order.totalPrice - fee;
    std::ostringstream data;
    data << std::fixed << std::setprecision(2)
         << "{\"orderId\":\"" << EscapeJson(orderId)
         << "\",\"originalPrice\":" << order.totalPrice
         << ",\"fee\":" << fee
         << ",\"refundAmount\":" << refundAmount
         << ",\"feeRate\":\"5%\",\"reason\":\"按订单实付金额计算\"}";
    return ApiResponse(data.str(), 0, "ok");
}

std::string TicketService::RefundOrderJson(const std::string& orderId, const std::string& requestBody) {
    (void)requestBody;
    std::lock_guard<std::mutex> lock(mutex_);
    Order* order = FindOrder(orderId);
    if (order == nullptr)
        return ApiResponse("null", 40401, "order not found");
    if (order->status == "refunded")
        return ApiResponse("null", 40003, "order already refunded");

    // 一张订单记录对应一个乘客；退票时将该车次、该席别的库存归还一张。
    Train* train = FindTrain(order->trainNo);
    if (train != nullptr) {
        const auto seatIt = std::find_if(train->seatTypes.begin(), train->seatTypes.end(), [&](const SeatType& seat) {
            return seat.type == order->seatType;
        });
        if (seatIt != train->seatTypes.end())
            ++seatIt->remain;
    }

    const double fee = order->totalPrice * 0.05;
    const double refundAmount = order->totalPrice - fee;
    order->status = "refunded";
    SaveData();
    std::ostringstream data;
    data << std::fixed << std::setprecision(2)
         << "{\"orderId\":\"" << EscapeJson(orderId)
         << "\",\"status\":\"refunded\",\"refundAmount\":" << refundAmount
         << ",\"fee\":" << fee
         << ",\"refundedAt\":\"2026-07-06T15:00:00\"}";
    return ApiResponse(data.str(), 0, "refund success");
}std::string TicketService::GetChangeOptionsJson(const std::string& orderId, const std::string& date) const {
    // 读取所需数据，并整理为接口需要的返回结果。
    std::lock_guard<std::mutex> lock(mutex_);
    const auto orderIt = std::find_if(orders_.begin(), orders_.end(), [&](const Order& item) { return item.orderId == orderId; });
    // 先确认目标数据存在，避免访问无效内容。
    if (orderIt == orders_.end())
        return ApiResponse("null", 40401, "order not found");
    const Order* order = &(*orderIt);

    // 未指定日期时，默认沿用原订单的乘车日期。
    const std::string targetDate = date.empty() ? order->date : date;
    std::vector<std::string> options;
    // 依次处理集合中的每一项数据。
    for (const auto& train : trains_) {
        // 根据当前条件决定是否进入下一步处理。
        if (train.trainNo == order->trainNo)
            continue;
        SegmentInfo segment;
        // 根据当前条件决定是否进入下一步处理。
        if (!TryGetSegment(train, order->fromStation, order->toStation, segment))
            continue;
        options.push_back(BuildChangeTrainOptionJson(train, segment, targetDate, order->totalPrice));
        // 检查数据数量或长度是否满足当前处理要求。
        if (options.size() >= 10)
            break;
    }

    std::ostringstream data;
    data << "{\"originalOrder\":" << OrderToJson(*order, true) << ",\"availableTrains\":[";
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < options.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            data << ',';
        data << options[i];
    }
    data << "]}";
    return ApiResponse(data.str(), 0, "ok");
}
std::string TicketService::ChangeOrderJson(const std::string& orderId, const std::string& requestBody) {
    std::lock_guard<std::mutex> lock(mutex_);
    Order* order = FindOrder(orderId);
    if (order == nullptr)
        return ApiResponse("null", 40401, "order not found");

    const std::string oldTrainNo = order->trainNo;
    const std::string oldSeatType = order->seatType;
    const std::string requestedTrainNo = ExtractString(requestBody, "newTrainNo");
    const std::string requestedDate = ExtractString(requestBody, "newDate");
    const std::string requestedSeatType = DecodeLooseUnicodeEscapes(ExtractString(requestBody, "newSeatType"));
    const std::string targetTrainNo = requestedTrainNo.empty() ? oldTrainNo : requestedTrainNo;
    const std::string targetSeatType = requestedSeatType.empty() ? oldSeatType : requestedSeatType;

    Train* targetTrain = FindTrain(targetTrainNo);
    if (targetTrain == nullptr)
        return ApiResponse("null", 40401, "train not found");
    SegmentInfo targetSegment;
    if (!TryGetSegment(*targetTrain, order->fromStation, order->toStation, targetSegment))
        return ApiResponse("null", 40001, "invalid train segment");
    auto targetSeatIt = std::find_if(targetTrain->seatTypes.begin(), targetTrain->seatTypes.end(), [&](const SeatType& seat) {
        return seat.type == targetSeatType;
    });
    if (targetSeatIt == targetTrain->seatTypes.end())
        return ApiResponse("null", 40402, "seat type not found");

    const bool keepsSameInventory = targetTrainNo == oldTrainNo && targetSeatType == oldSeatType;
    if (!keepsSameInventory && targetSeatIt->remain <= 0)
        return ApiResponse("null", 40002, "not enough seats");

    // 先确认新票可用，再回补旧票并扣减新票，避免改签失败时丢失原座位。
    if (!keepsSameInventory) {
        Train* oldTrain = FindTrain(oldTrainNo);
        if (oldTrain != nullptr) {
            const auto oldSeatIt = std::find_if(oldTrain->seatTypes.begin(), oldTrain->seatTypes.end(), [&](const SeatType& seat) {
                return seat.type == oldSeatType;
            });
            if (oldSeatIt != oldTrain->seatTypes.end())
                ++oldSeatIt->remain;
        }
        --targetSeatIt->remain;
    }

    order->trainNo = targetTrainNo;
    order->date = requestedDate.empty() ? order->date : requestedDate;
    order->seatType = targetSeatType;
    order->status = "changed";
    order->totalPrice = TripPrice(*targetSeatIt, targetSegment.distanceKm);
    SaveData();
    return ApiResponse("{\"orderId\":\"" + EscapeJson(orderId) + "\",\"oldTrainNo\":\"" + EscapeJson(oldTrainNo) + "\",\"newTrainNo\":\"" + EscapeJson(order->trainNo) + "\",\"priceDiff\":0.0,\"status\":\"changed\",\"changedAt\":\"2026-07-06T16:00:00\"}", 0, "change success");
}
