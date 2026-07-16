// 线路图模块：将车次相邻站点保存为邻接表，并使用 Dijkstra 计算最短换乘路径。

#include "../TicketService.h"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <limits>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

void TicketService::RebuildRouteGraph() {
    // 线路图是 trains_ 的派生数据；车次增删改后必须重建，避免查询到旧线路。
    // Rebuild the derived graph whenever the persisted train list changes.
    routeGraph_.clear();

    for (const auto& train : trains_) {
        // 每一对相邻站点构成有向边，保留车次和时刻以便最终生成可购票的分段。
        for (std::size_t i = 1; i < train.stops.size(); ++i) {
            const Stop& from = train.stops[i - 1];
            const Stop& to = train.stops[i];
            if (from.station.empty() || to.station.empty() || to.distanceKm <= 0)
                continue;

            routeGraph_[from.station].push_back({
                from.station,
                to.station,
                train.trainNo,
                from.depart,
                to.arrive,
                to.distanceKm
            });
        }
    }
}

std::vector<TicketService::GraphEdge> TicketService::FindShortestRouteDijkstra(const std::string& from, const std::string& to, const std::string& excludedEdge) const {
    const std::string source = ResolveStationName(from);
    const std::string target = ResolveStationName(to);
    if (source.empty() || target.empty() || source == target)
        return {};

    struct State {
        int distanceKm = 0;
        int arrivalMinutes = 0;
        std::string station;
        std::string currentTrainNo;
        std::vector<GraphEdge> path;
    };
    struct StateLater {
        bool operator()(const State& left, const State& right) const {
            if (left.distanceKm != right.distanceKm)
                return left.distanceKm > right.distanceKm;
            return left.arrivalMinutes > right.arrivalMinutes;
        }
    };
    struct Label {
        int distanceKm = 0;
        int arrivalMinutes = 0;
    };

    // 队列按累计里程、到达时刻排序。到达时刻不是权重，而是保证换乘可行的约束。
    std::priority_queue<State, std::vector<State>, StateLater> queue;
    std::unordered_map<std::string, std::vector<Label>> labels;
    queue.push({0, 0, source, "", {}});
    labels[source + "|"] = {{0, 0}};

    while (!queue.empty()) {
        State current = queue.top();
        queue.pop();
        if (current.station == target && !current.path.empty())
            return current.path;

        const auto graphIt = routeGraph_.find(current.station);
        if (graphIt == routeGraph_.end())
            continue;

        for (const auto& edge : graphIt->second) {
            const std::string edgeKey = edge.fromStation + "|" + edge.toStation + "|" + edge.trainNo + "|" + edge.departureTime;
            if (!excludedEdge.empty() && edgeKey == excludedEdge)
                continue;
            const int departureClock = ParseClockMinutes(edge.departureTime);
            const int arrivalClock = ParseClockMinutes(edge.arrivalTime);
            if (departureClock < 0 || arrivalClock < 0)
                continue;

            // 后续车次必须在上一段抵达以后出发；若当天已错过，则按次日同一车次计算。
            int departure = departureClock;
            if (!current.path.empty()) {
                while (departure < current.arrivalMinutes)
                    departure += 24 * 60;
            }
            int arrival = arrivalClock;
            while (arrival < departure)
                arrival += 24 * 60;

            State next;
            next.distanceKm = current.distanceKm + edge.distanceKm;
            next.arrivalMinutes = arrival;
            next.station = edge.toStation;
            next.currentTrainNo = edge.trainNo;
            next.path = current.path;
            next.path.push_back(edge);

            // 同一站、同一当前车次下，里程更长且到达更晚的状态不会产生更优方案。
            const std::string labelKey = next.station + "|" + next.currentTrainNo;
            auto& stationLabels = labels[labelKey];
            const bool dominated = std::any_of(stationLabels.begin(), stationLabels.end(), [&](const Label& label) {
                return label.distanceKm <= next.distanceKm && label.arrivalMinutes <= next.arrivalMinutes;
            });
            if (dominated)
                continue;
            stationLabels.erase(std::remove_if(stationLabels.begin(), stationLabels.end(), [&](const Label& label) {
                return next.distanceKm <= label.distanceKm && next.arrivalMinutes <= label.arrivalMinutes;
            }), stationLabels.end());
            stationLabels.push_back({next.distanceKm, next.arrivalMinutes});
            queue.push(std::move(next));
        }
    }

    return {};
}
std::string TicketService::BuildGraphTransferRouteJson(const std::vector<GraphEdge>& path, const std::string& algorithm) const {
    if (path.empty())
        return "";

    struct RouteLeg {
        GraphEdge firstEdge;
        GraphEdge lastEdge;
        int distanceKm = 0;
    };

    // 同一车次连续经过多条图边时，合并成一个购票区间，避免用户重复下单。
    std::vector<RouteLeg> legs;
    for (const auto& edge : path) {
        if (!legs.empty() && legs.back().lastEdge.trainNo == edge.trainNo && legs.back().lastEdge.toStation == edge.fromStation) {
            legs.back().lastEdge = edge;
            legs.back().distanceKm += edge.distanceKm;
        }
        else {
            legs.push_back({edge, edge, edge.distanceKm});
        }
    }

    // 单趟车已由 directTrains 返回；这里只保留至少换乘一次的方案。
    // A single train is already represented by directTrains.
    if (legs.size() < 2)
        return "";

    double totalPrice = 0.0;
    std::vector<std::string> transferStations;
    for (std::size_t i = 0; i < legs.size(); ++i) {
        const Train* train = FindTrain(legs[i].firstEdge.trainNo);
        if (train != nullptr) {
            const SeatType* cheapestSeat = CheapestSeatType(*train);
            if (cheapestSeat != nullptr)
                totalPrice += TripPrice(*cheapestSeat, legs[i].distanceKm);
        }
        if (i > 0)
            transferStations.push_back(legs[i - 1].lastEdge.toStation);
    }

    // 统一成分钟并处理跨零点，保证总耗时包含候车时间。
    const int departure = ParseClockMinutes(legs.front().firstEdge.departureTime);
    int currentArrival = ParseClockMinutes(legs.front().lastEdge.arrivalTime);
    while (currentArrival >= 0 && departure >= 0 && currentArrival < departure)
        currentArrival += 24 * 60;
    for (std::size_t i = 1; i < legs.size(); ++i) {
        int nextDeparture = ParseClockMinutes(legs[i].firstEdge.departureTime);
        int nextArrival = ParseClockMinutes(legs[i].lastEdge.arrivalTime);
        while (nextDeparture >= 0 && currentArrival >= 0 && nextDeparture < currentArrival)
            nextDeparture += 24 * 60;
        while (nextArrival >= 0 && nextDeparture >= 0 && nextArrival < nextDeparture)
            nextArrival += 24 * 60;
        currentArrival = nextArrival;
    }
    const int totalMinutes = departure >= 0 && currentArrival >= departure ? currentArrival - departure : 0;

    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{\"algorithm\":\"" << EscapeJson(algorithm)
         << "\",\"totalDuration\":\"" << EscapeJson(FormatDurationMinutes(totalMinutes))
         << "\",\"totalPrice\":" << totalPrice << ",\"transferStation\":\"";
    for (std::size_t i = 0; i < transferStations.size(); ++i) {
        if (i > 0)
            json << " / ";
        json << EscapeJson(transferStations[i]);
    }
    json << "\",\"legs\":[";

    for (std::size_t i = 0; i < legs.size(); ++i) {
        if (i > 0)
            json << ',';
        const RouteLeg& leg = legs[i];
        const Train* train = FindTrain(leg.firstEdge.trainNo);
        const SeatType* cheapestSeat = train == nullptr ? nullptr : CheapestSeatType(*train);
        const double legPrice = cheapestSeat == nullptr ? 0.0 : TripPrice(*cheapestSeat, leg.distanceKm);
        json << "{\"trainNo\":\"" << EscapeJson(leg.firstEdge.trainNo)
             << "\",\"fromStation\":\"" << EscapeJson(leg.firstEdge.fromStation)
             << "\",\"toStation\":\"" << EscapeJson(leg.lastEdge.toStation)
             << "\",\"departureTime\":\"" << EscapeJson(leg.firstEdge.departureTime)
             << "\",\"arrivalTime\":\"" << EscapeJson(leg.lastEdge.arrivalTime)
             << "\",\"seatType\":\"" << EscapeJson(cheapestSeat == nullptr ? "" : cheapestSeat->type)
             << "\",\"price\":" << legPrice
             << ",\"distanceKm\":" << leg.distanceKm
             // seatTypes 是现有前端 EXE 的字段名；seatOptions 同时保留以兼容后续调用方。
             << ",\"seatTypes\":" << (train == nullptr ? "[]" : SeatTypesForSegmentJson(*train, leg.distanceKm))
             << ",\"seatOptions\":" << (train == nullptr ? "[]" : SeatTypesForSegmentJson(*train, leg.distanceKm))
             << '}';
    }
    json << "]}";
    return json.str();
}