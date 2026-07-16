// 文件说明：管理模块：提供车站和车次的新增、编辑与删除功能。

#include "../TicketService.h"

#include <algorithm>
#include <mutex>

std::string TicketService::SaveStationJson(const std::string& requestBody, const std::string& editingCode) {
    // 管理员新增或编辑车站后，更新内存数据并保存到文件。
    std::lock_guard<std::mutex> lock(mutex_);
    Station station {
        ExtractString(requestBody, "code"),
        DecodeLooseUnicodeEscapes(ExtractString(requestBody, "name")),
        DecodeLooseUnicodeEscapes(ExtractString(requestBody, "city"))
    };
    // 先确认目标数据存在，避免访问无效内容。
    if (station.code.empty() && !editingCode.empty())
        station.code = editingCode;
    auto it = std::find_if(stations_.begin(), stations_.end(), [&](const Station& item) { return item.code == (editingCode.empty() ? station.code : editingCode); });
    // 先确认目标数据存在，避免访问无效内容。
    if (it == stations_.end())
        stations_.push_back(station);
    else *it = station;
    SaveData();

    return ApiResponse("null", 0, editingCode.empty() ? "station added" : "station updated");
}

std::string TicketService::DeleteStationJson(const std::string& code) {
    // 处理删除或退款操作，并返回处理结果。
    std::lock_guard<std::mutex> lock(mutex_);
    stations_.erase(std::remove_if(stations_.begin(), stations_.end(), [&](const Station& station) { return station.code == code; }), stations_.end());
    SaveData();
    return ApiResponse("null", 0, "station deleted");
}

std::string TicketService::SaveTrainJson(const std::string& requestBody, const std::string& editingTrainNo) {
    // 新增或编辑车次，并将座位和途经站信息一同保存。
    std::lock_guard<std::mutex> lock(mutex_);
    Train train;
    train.trainNo = ExtractString(requestBody, "trainNo");
    train.fromStation = ExtractString(requestBody, "fromStation");
    train.toStation = ExtractString(requestBody, "toStation");
    train.departureTime = ExtractString(requestBody, "departureTime");
    train.arrivalTime = ExtractString(requestBody, "arrivalTime");
    train.duration = ExtractString(requestBody, "duration");
    train.seatTypes = ParseSeatTypesFromJson(requestBody);
    // 先确认目标数据存在，避免访问无效内容。
    if (train.seatTypes.empty()) {
        train.seatTypes = {{"Second Class", 0.42, 100}};
    }
    train.stops = ParseStopsFromJson(requestBody);
    // 先确认目标数据存在，避免访问无效内容。
    if (train.stops.empty()) {
        train.stops = {{train.fromStation, "-", train.departureTime, 1, 0}, {train.toStation, train.arrivalTime, "-", 2, 0}};
    }
    // 先确认目标数据存在，避免访问无效内容。
    if (train.trainNo.empty() && !editingTrainNo.empty())
        train.trainNo = editingTrainNo;
    auto it = std::find_if(trains_.begin(), trains_.end(), [&](const Train& item) { return item.trainNo == (editingTrainNo.empty() ? train.trainNo : editingTrainNo); });
    // 先确认目标数据存在，避免访问无效内容。
    if (it == trains_.end())
        trains_.push_back(train);
    else *it = train;
    RebuildRouteGraph();
    SaveData();

    return ApiResponse("null", 0, editingTrainNo.empty() ? "train added" : "train updated");
}

std::string TicketService::DeleteTrainJson(const std::string& trainNo) {
    // 处理删除或退款操作，并返回处理结果。
    std::lock_guard<std::mutex> lock(mutex_);
    trains_.erase(std::remove_if(trains_.begin(), trains_.end(), [&](const Train& train) { return train.trainNo == trainNo; }), trains_.end());
    RebuildRouteGraph();
    SaveData();
    return ApiResponse("null", 0, "train deleted");
}

