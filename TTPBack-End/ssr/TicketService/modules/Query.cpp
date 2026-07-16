// 文件说明：查询模块：负责直达车次、换乘方案和余票信息查询。

#include "../TicketService.h"
#include "../../Logger.h"

#include <algorithm>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <tuple>

std::string TicketService::GetStationsJson() const {
    // 读取所需数据，并整理为接口需要的返回结果。
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream data;
    data << '[';
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < stations_.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            data << ',';
        data << StationToJson(stations_[i]);
    }
    data << ']';
    return ApiResponse(data.str(), 0, "ok");
}

std::string TicketService::GetAllTrainsJson() const {
    // 读取所需数据，并整理为接口需要的返回结果。
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream data;
    data << '[';
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < trains_.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            data << ',';
        data << TrainToAdminListJson(trains_[i], "2026-07-10");
    }
    data << ']';
    return ApiResponse(data.str(), 0, "ok");
}

std::string TicketService::SearchTrainsJson(const std::string& from, const std::string& to, const std::string& date) const {
    // 查找能够从出发站到达目的站的直达车次。
    std::lock_guard<std::mutex> lock(mutex_);

    // 先确认目标数据存在，避免访问无效内容。
    if ((!from.empty() && !IsKnownStation(from)) || (!to.empty() && !IsKnownStation(to))) {
        Logger::Warn("SearchTrainsJson invalid station from=\"" + EscapeJson(from) + "\" to=\"" + EscapeJson(to) + "\"");
        return ApiResponse("[]", 0, "ok");
    }

    struct TrainCandidate {
        int durationMinutes = 0;
        double minPrice = 0.0;
        int departureMinutes = 0;
        std::string trainNo;
        std::string json;
    };

    // 收集符合区间条件的车次，并预先计算排序所需的价格和时间。
    std::vector<TrainCandidate> candidates;
    // 依次处理集合中的每一项数据。
    for (const auto& train : trains_) {
        SegmentInfo segment;
        const bool allTrains = from.empty() && to.empty();
        const bool match = allTrains ? true : TryGetQuerySegment(train, from, to, segment);
        // 根据当前条件决定是否进入下一步处理。
        if (!match)
            continue;
        const SeatType* seat = CheapestSeatType(train);
        const int distanceKm = allTrains ? TotalDistanceKm(train) : segment.distanceKm;
        const double minPrice = seat == nullptr ? 0.0 : TripPrice(*seat, distanceKm);
        const int departureMinutes = ParseClockMinutes(allTrains ? train.departureTime : train.stops[segment.fromIndex].depart);
        const int arrivalMinutes = ParseClockMinutes(allTrains ? train.arrivalTime : train.stops[segment.toIndex].arrive);
        const int durationMinutes = ElapsedMinutes(departureMinutes, arrivalMinutes);

        candidates.push_back({
            durationMinutes,
            minPrice,
            departureMinutes,
            train.trainNo,
            allTrains ? TrainToJson(train, date, false) : BuildSegmentTrainJson(train, segment, date)
        });
    }

    // 优先展示耗时短、票价低、出发早的车次。
    std::sort(candidates.begin(), candidates.end(), [](const TrainCandidate& left, const TrainCandidate& right) {
        return std::tie(left.durationMinutes, left.minPrice, left.departureMinutes, left.trainNo)
             < std::tie(right.durationMinutes, right.minPrice, right.departureMinutes, right.trainNo);
    });

    std::ostringstream data;
    data << '[';
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            data << ',';
        data << candidates[i].json;
    }
    data << ']';
    return ApiResponse(data.str(), 0, "ok");
}

std::string TicketService::GetTrainDetailJson(const std::string& trainNo) const {
    // 读取所需数据，并整理为接口需要的返回结果。
    std::lock_guard<std::mutex> lock(mutex_);
    const Train* train = FindTrain(trainNo);
    // 先确认目标数据存在，避免访问无效内容。
    if (train == nullptr)
        return ApiResponse("null", 40401, "train not found");
    return ApiResponse(TrainToJson(*train, "2026-07-10", true), 0, "ok");
}

std::string TicketService::GetTransferRoutesJson(const std::string& from, const std::string& to, const std::string& date) const {
    // 直达车次沿用原有结果；换乘方案改由线路图最短路径算法生成。
    (void)date;
    std::lock_guard<std::mutex> lock(mutex_);

    if (from.empty() || to.empty() || !IsKnownStation(from) || !IsKnownStation(to)) {
        Logger::Warn("GetTransferRoutesJson invalid station from=\"" + EscapeJson(from) + "\" to=\"" + EscapeJson(to) + "\"");
        return ApiResponse("{\"directTrains\":[],\"transferRoutes\":[]}", 0, "ok");
    }

    struct DirectCandidate {
        int durationMinutes = 0;
        double minPrice = 0.0;
        int departureMinutes = 0;
        std::string trainNo;
        std::string json;
    };

    std::vector<DirectCandidate> directEntries;
    for (const auto& train : trains_) {
        SegmentInfo segment;
        if (!TryGetSegment(train, from, to, segment))
            continue;
        const SeatType* seat = CheapestSeatType(train);
        const int departure = ParseClockMinutes(train.stops[segment.fromIndex].depart);
        const int arrival = ParseClockMinutes(train.stops[segment.toIndex].arrive);
        directEntries.push_back({
            ElapsedMinutes(departure, arrival),
            seat == nullptr ? 0.0 : TripPrice(*seat, segment.distanceKm),
            departure,
            train.trainNo,
            BuildTransferDirectJson(train, segment)
        });
    }
    std::sort(directEntries.begin(), directEntries.end(), [](const DirectCandidate& left, const DirectCandidate& right) {
        return std::tie(left.durationMinutes, left.minPrice, left.departureMinutes, left.trainNo)
             < std::tie(right.durationMinutes, right.minPrice, right.departureMinutes, right.trainNo);
    });

    // 图中的边来自所有车次的相邻站点；Dijkstra 找到累计里程最短的可达路径。
    const std::vector<GraphEdge> shortestPath = FindShortestRouteDijkstra(from, to);
    const std::string sourceName = ResolveStationName(from);
    const auto graphIt = routeGraph_.find(sourceName);
    const std::size_t outgoingEdges = graphIt == routeGraph_.end() ? 0 : graphIt->second.size();
    Logger::Info("Dijkstra transfer from=" + EscapeJson(sourceName) + " to=" + EscapeJson(ResolveStationName(to))
        + " outgoingEdges=" + std::to_string(outgoingEdges)
        + " pathEdges=" + std::to_string(shortestPath.size()));
    // 将图边合并为可购票的车次分段，并保持前端既有 transferRoutes 数据结构。
    std::vector<std::string> routeJsons;
    auto addRouteJson = [&routeJsons](const std::string& candidate) {
        // 只有完整的 JSON 对象才能进入数组，避免空串或异常内容破坏整个接口响应。
        if (candidate.size() < 2 || candidate.front() != '{' || candidate.back() != '}')
            return;
        if (std::find(routeJsons.begin(), routeJsons.end(), candidate) == routeJsons.end())
            routeJsons.push_back(candidate);
    };
    if (!shortestPath.empty())
        addRouteJson(BuildGraphTransferRouteJson(shortestPath, "dijkstra"));
    // 对最短方案逐边屏蔽后重跑 Dijkstra，收集不同的可行备选方案。
    for (const auto& edge : shortestPath) {
        if (routeJsons.size() >= 5) break;
        const std::string excluded = edge.fromStation + "|" + edge.toStation + "|" + edge.trainNo + "|" + edge.departureTime;
        const auto alternative = FindShortestRouteDijkstra(from, to, excluded);
        const std::string candidate = BuildGraphTransferRouteJson(alternative, "dijkstra");
        addRouteJson(candidate);
    }

    std::ostringstream data;
    data << "{\"directTrains\":[";
    for (std::size_t i = 0; i < directEntries.size(); ++i) {
        if (i > 0)
            data << ',';
        data << directEntries[i].json;
    }
    data << "],\"transferRoutes\":[";
    for (std::size_t i = 0; i < routeJsons.size(); ++i) {
        if (i > 0) data << ',';
        data << routeJsons[i];
    }
    data << "]}";
    return ApiResponse(data.str(), 0, "ok");
}
std::string TicketService::GetTicketsJson(const std::string& from, const std::string& to, const std::string& date) const {
    // 读取所需数据，并整理为接口需要的返回结果。
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string fromName = ResolveStationName(from);
    const std::string toName = ResolveStationName(to);

    // 先确认目标数据存在，避免访问无效内容。
    if ((!from.empty() && !IsKnownStation(from)) || (!to.empty() && !IsKnownStation(to))) {
        Logger::Warn("GetTicketsJson invalid station from=\"" + EscapeJson(from) + "\" to=\"" + EscapeJson(to) + "\"");
        return ApiResponse("{\"from\":\"" + EscapeJson(fromName) + "\",\"to\":\"" + EscapeJson(toName) + "\",\"date\":\"" + EscapeJson(date) + "\",\"trains\":[]}", 0, "ok");
    }

    struct TicketCandidate {
        int durationMinutes = 0;
        double minPrice = 0.0;
        int departureMinutes = 0;
        std::string trainNo;
        std::string json;
    };

    std::vector<TicketCandidate> candidates;
    // 依次处理集合中的每一项数据。
    for (const auto& train : trains_) {
        SegmentInfo segment;
        // 根据当前条件决定是否进入下一步处理。
        if (!TryGetQuerySegment(train, from, to, segment))
            continue;
        const SeatType* seat = CheapestSeatType(train);
        const double minPrice = seat == nullptr ? 0.0 : TripPrice(*seat, segment.distanceKm);
        const int departureMinutes = ParseClockMinutes(train.stops[segment.fromIndex].depart);
        const int arrivalMinutes = ParseClockMinutes(train.stops[segment.toIndex].arrive);
        candidates.push_back({
            ElapsedMinutes(departureMinutes, arrivalMinutes),
            minPrice,
            departureMinutes,
            train.trainNo,
            BuildSegmentTrainJson(train, segment, date)
        });
    }

    // 优先展示耗时短、票价低、出发早的车次。
    std::sort(candidates.begin(), candidates.end(), [](const TicketCandidate& left, const TicketCandidate& right) {
        return std::tie(left.durationMinutes, left.minPrice, left.departureMinutes, left.trainNo)
             < std::tie(right.durationMinutes, right.minPrice, right.departureMinutes, right.trainNo);
    });

    std::ostringstream trainsJson;
    trainsJson << '[';
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            trainsJson << ',';
        trainsJson << candidates[i].json;
    }
    trainsJson << ']';

    std::ostringstream data;
    data << "{\"from\":\"" << EscapeJson(fromName) << "\",\"to\":\"" << EscapeJson(toName)
         << "\",\"date\":\"" << EscapeJson(date) << "\",\"trains\":" << trainsJson.str() << '}';
    return ApiResponse(data.str(), 0, "ok");
}

