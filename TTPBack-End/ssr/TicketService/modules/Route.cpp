// 文件说明：路线工具模块：负责区间、票价、时长和车次展示数据计算。

#include "../TicketService.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

int TicketService::TotalDistanceKm(const Train& train) {
    // 计算行程、价格或时间等业务数据。
    int total = 0;
    // 依次处理集合中的每一项数据。
    for (const auto& stop : train.stops) {
        total += stop.distanceKm;
    }
    return total;
}

double TicketService::TripPrice(const SeatType& seat, int totalDistanceKm) {
    // 计算行程、价格或时间等业务数据。
    return seat.price > 10.0 || totalDistanceKm <= 0 ? seat.price : seat.price * totalDistanceKm;
}

std::string TicketService::StationToJson(const Station& station) const {
    // 组合和转换字段，生成程序或前端使用的数据。
    std::ostringstream edges;
    edges << '[';
    bool first = true;
    std::vector<std::string> emittedEdges;

    auto appendEdge = [&](const std::string& from, const std::string& to, int distance) {
        // 发现异常或无效结果时，及时结束当前分支。
        if (from != station.name || distance <= 0)
            return;
        const std::string edgeKey = from + "->" + to;
        // 先确认目标数据存在，避免访问无效内容。
        if (std::find(emittedEdges.begin(), emittedEdges.end(), edgeKey) != emittedEdges.end())
            return;
        emittedEdges.push_back(edgeKey);
        // 根据当前条件决定是否进入下一步处理。
        if (!first)
            edges << ',';
        edges << "{\"fromCode\":\"" << EscapeJson(station.code) << "\",\"toCode\":\"";
        std::string toCode = to;
        // 依次处理集合中的每一项数据。
        for (const auto& item : stations_) {
            // 根据当前条件决定是否进入下一步处理。
            if (item.name == to) {
                toCode = item.code;
                break;
            }
        }
        edges << EscapeJson(toCode) << "\",\"distance\":" << distance << '}';
        first = false;
    };

    // 依次处理集合中的每一项数据。
    for (const auto& train : trains_) {
        // 依次处理集合中的每一项数据。
        for (std::size_t i = 1; i < train.stops.size(); ++i) {
            appendEdge(train.stops[i - 1].station, train.stops[i].station, train.stops[i].distanceKm);
            appendEdge(train.stops[i].station, train.stops[i - 1].station, train.stops[i].distanceKm);
        }
    }
    edges << ']';

    return "{\"code\":\"" + EscapeJson(station.code) + "\",\"name\":\"" + EscapeJson(station.name) + "\",\"city\":\"" + EscapeJson(station.city) + "\",\"edges\":" + edges.str() + "}";
}
std::string TicketService::TrainToJson(const Train& train, const std::string& date, bool includeStops) {
    // 组合和转换字段，生成程序或前端使用的数据。
    const int totalDistance = TotalDistanceKm(train);
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{\"trainNo\":\"" << EscapeJson(train.trainNo) << "\",\"fromStation\":\"" << EscapeJson(train.fromStation)
         << "\",\"toStation\":\"" << EscapeJson(train.toStation) << "\",\"departureTime\":\"" << EscapeJson(train.departureTime)
         << "\",\"arrivalTime\":\"" << EscapeJson(train.arrivalTime) << "\",\"duration\":\"" << EscapeJson(train.duration)
         << "\",\"date\":\"" << EscapeJson(date) << "\",\"totalDistanceKm\":" << totalDistance << ",\"seatTypes\":[";

    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < train.seatTypes.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            json << ',';
        json << "{\"type\":\"" << EscapeJson(train.seatTypes[i].type) << "\",\"price\":" << train.seatTypes[i].price << ",\"remain\":" << train.seatTypes[i].remain << '}';
    }
    json << ']';

    // 根据当前条件决定是否进入下一步处理。
    if (includeStops) {
        json << ",\"stops\":[";
        // 依次处理集合中的每一项数据。
        for (std::size_t i = 0; i < train.stops.size(); ++i) {
            // 根据当前条件决定是否进入下一步处理。
            if (i > 0)
                json << ',';
            json << "{\"station\":\"" << EscapeJson(train.stops[i].station) << "\",\"arrive\":\"" << EscapeJson(train.stops[i].arrive)
                 << "\",\"depart\":\"" << EscapeJson(train.stops[i].depart) << "\",\"seq\":" << train.stops[i].seq
                 << ",\"distanceKm\":" << train.stops[i].distanceKm << '}';
        }
        json << ']';
    }

    json << '}';
    return json.str();
}

std::string TicketService::TrainToAdminListJson(const Train& train, const std::string& date) {
    // 组合和转换字段，生成程序或前端使用的数据。
    const int totalDistance = TotalDistanceKm(train);
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{\"trainNo\":\"" << EscapeJson(train.trainNo) << "\",\"fromStation\":\"" << EscapeJson(train.fromStation)
         << "\",\"toStation\":\"" << EscapeJson(train.toStation) << "\",\"departureTime\":\"" << EscapeJson(train.departureTime)
         << "\",\"arrivalTime\":\"" << EscapeJson(train.arrivalTime) << "\",\"duration\":\"" << EscapeJson(train.duration)
         << "\",\"date\":\"" << EscapeJson(date) << "\",\"totalDistanceKm\":" << totalDistance << ",\"seatTypes\":[";

    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < train.seatTypes.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            json << ',';
        json << "{\"type\":\"" << EscapeJson(train.seatTypes[i].type)
             << "\",\"price\":" << TripPrice(train.seatTypes[i], totalDistance)
             << ",\"remain\":" << train.seatTypes[i].remain << '}';
    }
    json << "]}";
    return json.str();
}
bool TicketService::IsKnownStation(const std::string& codeOrName) const {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    if (codeOrName.empty())
        return false;
    return std::any_of(stations_.begin(), stations_.end(), [&](const Station& station) {
        return station.code == codeOrName || station.name == codeOrName;
    });
}

std::string TicketService::ResolveStationName(const std::string& codeOrName) const {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    if (codeOrName.empty())
        return "";
    // 依次处理集合中的每一项数据。
    for (const auto& station : stations_) {
        // 根据当前条件决定是否进入下一步处理。
        if (station.code == codeOrName || station.name == codeOrName)
            return station.name;
    }

    return codeOrName;
}

bool TicketService::TryGetSegment(const Train& train, const std::string& from, const std::string& to, SegmentInfo& segment) const {
    // 在列车经停站中定位出发站和到达站，并计算该区间里程。
    const std::string fromName = ResolveStationName(from);
    const std::string toName = ResolveStationName(to);
    // 先确认目标数据存在，避免访问无效内容。
    if (fromName.empty() || toName.empty())
        return false;
    // 依次处理集合中的每一项数据。
    for (std::size_t fromIndex = 0; fromIndex < train.stops.size(); ++fromIndex) {
        // 根据当前条件决定是否进入下一步处理。
        if (train.stops[fromIndex].station != fromName)
            continue;
        // 依次处理集合中的每一项数据。
        for (std::size_t toIndex = fromIndex + 1; toIndex < train.stops.size(); ++toIndex) {
            // 根据当前条件决定是否进入下一步处理。
            if (train.stops[toIndex].station != toName)
                continue;
            segment = {fromIndex, toIndex, SegmentDistanceKm(train, fromIndex, toIndex)};
            return segment.distanceKm > 0;
        }
    }

    return false;
}


bool TicketService::TryGetQuerySegment(const Train& train, const std::string& from, const std::string& to, SegmentInfo& segment) const {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    const std::string fromName = ResolveStationName(from);
    const std::string toName = ResolveStationName(to);

    // 先确认目标数据存在，避免访问无效内容。
    if (!fromName.empty() && !toName.empty())
        return TryGetSegment(train, from, to, segment);
    // 先确认目标数据存在，避免访问无效内容。
    if (!fromName.empty()) {
        // 依次处理集合中的每一项数据。
        for (std::size_t fromIndex = 0; fromIndex + 1 < train.stops.size(); ++fromIndex) {
            // 根据当前条件决定是否进入下一步处理。
            if (train.stops[fromIndex].station != fromName)
                continue;
            const std::size_t toIndex = train.stops.size() - 1;
            segment = {fromIndex, toIndex, SegmentDistanceKm(train, fromIndex, toIndex)};
            return segment.distanceKm > 0;
        }
        return false;
    }

    // 先确认目标数据存在，避免访问无效内容。
    if (!toName.empty()) {
        // 依次处理集合中的每一项数据。
        for (std::size_t toIndex = 1; toIndex < train.stops.size(); ++toIndex) {
            // 根据当前条件决定是否进入下一步处理。
            if (train.stops[toIndex].station != toName)
                continue;
            segment = {0, toIndex, SegmentDistanceKm(train, 0, toIndex)};
            return segment.distanceKm > 0;
        }
        return false;
    }

    return false;
}

std::string TicketService::BuildChangeTrainOptionJson(const Train& train, const SegmentInfo& segment, const std::string& date, double originalPrice) const {
    // 组合和转换字段，生成程序或前端使用的数据。
    const SeatType* cheapestSeat = CheapestSeatType(train);
    const double minPrice = cheapestSeat == nullptr ? 0.0 : TripPrice(*cheapestSeat, segment.distanceKm);
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{\"trainNo\":\"" << EscapeJson(train.trainNo)
         << "\",\"fromStation\":\"" << EscapeJson(train.stops[segment.fromIndex].station)
         << "\",\"toStation\":\"" << EscapeJson(train.stops[segment.toIndex].station)
         << "\",\"departureTime\":\"" << EscapeJson(train.stops[segment.fromIndex].depart)
         << "\",\"arrivalTime\":\"" << EscapeJson(train.stops[segment.toIndex].arrive)
         << "\",\"date\":\"" << EscapeJson(date)
         << "\",\"priceDiff\":" << (minPrice - originalPrice)
         << ",\"seatTypes\":" << SeatTypesForSegmentJson(train, segment.distanceKm)
         << "}";
    return json.str();
}

std::string TicketService::BuildSegmentTrainJson(const Train& train, const SegmentInfo& segment, const std::string& date) const {
    // 为某一段行程计算价格、时长和余票，并输出给前端。
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{\"trainNo\":\"" << EscapeJson(train.trainNo)
         << "\",\"fromStation\":\"" << EscapeJson(train.stops[segment.fromIndex].station)
         << "\",\"toStation\":\"" << EscapeJson(train.stops[segment.toIndex].station)
         << "\",\"departureTime\":\"" << EscapeJson(train.stops[segment.fromIndex].depart)
         << "\",\"arrivalTime\":\"" << EscapeJson(train.stops[segment.toIndex].arrive)
         << "\",\"duration\":\"" << EscapeJson(SegmentDuration(train, segment.fromIndex, segment.toIndex))
         << "\",\"date\":\"" << EscapeJson(date)
         << "\",\"totalDistanceKm\":" << segment.distanceKm
         << ",\"seatTypes\":[";

    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < train.seatTypes.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            json << ',';
        json << "{\"type\":\"" << EscapeJson(train.seatTypes[i].type)
             << "\",\"price\":" << train.seatTypes[i].price
             << ",\"remain\":" << train.seatTypes[i].remain << '}';
    }
    json << "]}";
    return json.str();
}


std::string TicketService::SeatTypesForSegmentJson(const Train& train, int distanceKm) {
    // 执行该函数对应的处理，并返回结果。
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << '[';
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < train.seatTypes.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            json << ',';
        json << "{\"type\":\"" << EscapeJson(train.seatTypes[i].type)
             << "\",\"price\":" << TripPrice(train.seatTypes[i], distanceKm)
             << ",\"remain\":" << train.seatTypes[i].remain << '}';
    }
    json << ']';
    return json.str();
}
std::string TicketService::BuildTransferDirectJson(const Train& train, const SegmentInfo& segment) const {
    // 组合和转换字段，生成程序或前端使用的数据。
    const SeatType* seat = CheapestSeatType(train);
    const double minPrice = seat == nullptr ? 0.0 : TripPrice(*seat, segment.distanceKm);

    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{\"trainNo\":\"" << EscapeJson(train.trainNo)
         << "\",\"departureTime\":\"" << EscapeJson(train.stops[segment.fromIndex].depart)
         << "\",\"arrivalTime\":\"" << EscapeJson(train.stops[segment.toIndex].arrive)
         << "\",\"duration\":\"" << EscapeJson(SegmentDuration(train, segment.fromIndex, segment.toIndex))
         << "\",\"minPrice\":" << minPrice
         << ",\"seatTypes\":" << SeatTypesForSegmentJson(train, segment.distanceKm)
         << ",\"totalDistanceKm\":" << segment.distanceKm << '}';
    return json.str();
}

std::string TicketService::BuildTransferRouteJson(const Train& firstTrain, const SegmentInfo& firstSegment, const Train& secondTrain, const SegmentInfo& secondSegment) const {
    // 组合和转换字段，生成程序或前端使用的数据。
    const SeatType* firstSeat = CheapestSeatType(firstTrain);
    const SeatType* secondSeat = CheapestSeatType(secondTrain);
    const double firstPrice = firstSeat == nullptr ? 0.0 : TripPrice(*firstSeat, firstSegment.distanceKm);
    const double secondPrice = secondSeat == nullptr ? 0.0 : TripPrice(*secondSeat, secondSegment.distanceKm);
    const int departMinutes = ParseClockMinutes(firstTrain.stops[firstSegment.fromIndex].depart);
    int firstArrivalAbs = ParseClockMinutes(firstTrain.stops[firstSegment.toIndex].arrive);
    int secondDepartureAbs = ParseClockMinutes(secondTrain.stops[secondSegment.fromIndex].depart);
    int secondArrivalAbs = ParseClockMinutes(secondTrain.stops[secondSegment.toIndex].arrive);
    // 持续处理，直到满足结束条件。
    while (firstArrivalAbs < departMinutes)
        firstArrivalAbs += 24 * 60;
    // 持续处理，直到满足结束条件。
    while (secondDepartureAbs < firstArrivalAbs)
        secondDepartureAbs += 24 * 60;
    // 持续处理，直到满足结束条件。
    while (secondArrivalAbs < secondDepartureAbs)
        secondArrivalAbs += 24 * 60;
    const int totalMinutes = (departMinutes >= 0 && secondArrivalAbs >= departMinutes) ? (secondArrivalAbs - departMinutes) : 0;

    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{\"totalDuration\":\"" << EscapeJson(FormatDurationMinutes(totalMinutes))
         << "\",\"totalPrice\":" << (firstPrice + secondPrice)
         << ",\"transferStation\":\"" << EscapeJson(firstTrain.stops[firstSegment.toIndex].station)
         << "\",\"legs\":["
         << "{\"trainNo\":\"" << EscapeJson(firstTrain.trainNo)
         << "\",\"fromStation\":\"" << EscapeJson(firstTrain.stops[firstSegment.fromIndex].station)
         << "\",\"toStation\":\"" << EscapeJson(firstTrain.stops[firstSegment.toIndex].station)
         << "\",\"departureTime\":\"" << EscapeJson(firstTrain.stops[firstSegment.fromIndex].depart)
         << "\",\"arrivalTime\":\"" << EscapeJson(firstTrain.stops[firstSegment.toIndex].arrive)
         << "\",\"seatType\":\"" << EscapeJson(firstSeat == nullptr ? "" : firstSeat->type)
         << "\",\"price\":" << firstPrice
         << ",\"distanceKm\":" << firstSegment.distanceKm
         << ",\"seatTypes\":" << SeatTypesForSegmentJson(firstTrain, firstSegment.distanceKm) << "},"
         << "{\"trainNo\":\"" << EscapeJson(secondTrain.trainNo)
         << "\",\"fromStation\":\"" << EscapeJson(secondTrain.stops[secondSegment.fromIndex].station)
         << "\",\"toStation\":\"" << EscapeJson(secondTrain.stops[secondSegment.toIndex].station)
         << "\",\"departureTime\":\"" << EscapeJson(secondTrain.stops[secondSegment.fromIndex].depart)
         << "\",\"arrivalTime\":\"" << EscapeJson(secondTrain.stops[secondSegment.toIndex].arrive)
         << "\",\"seatType\":\"" << EscapeJson(secondSeat == nullptr ? "" : secondSeat->type)
         << "\",\"price\":" << secondPrice
         << ",\"distanceKm\":" << secondSegment.distanceKm
         << ",\"seatTypes\":" << SeatTypesForSegmentJson(secondTrain, secondSegment.distanceKm) << "}]";
    json << '}';
    return json.str();
}

int TicketService::SegmentDistanceKm(const Train& train, std::size_t fromIndex, std::size_t toIndex) {
    // 计算行程、价格或时间等业务数据。
    int total = 0;
    // 依次处理集合中的每一项数据。
    for (std::size_t i = fromIndex + 1; i <= toIndex && i < train.stops.size(); ++i) {
        total += train.stops[i].distanceKm;
    }
    return total;
}

std::string TicketService::SegmentDuration(const Train& train, std::size_t fromIndex, std::size_t toIndex) {
    // 计算行程、价格或时间等业务数据。
    const int depart = ParseClockMinutes(train.stops[fromIndex].depart);
    const int arrive = ParseClockMinutes(train.stops[toIndex].arrive);
    const int elapsed = ElapsedMinutes(depart, arrive);
    // 根据当前条件决定是否进入下一步处理。
    if (elapsed > 0)
        return FormatDurationMinutes(elapsed);
    return train.duration;
}

int TicketService::ParseClockMinutes(const std::string& hhmm) {
    // 解析输入内容，转换为程序内部可用的数据。
    const auto colon = hhmm.find(':');
    // 根据当前条件决定是否进入下一步处理。
    if (colon == std::string::npos)
        return -1;
    try {
        return (std::stoi(hhmm.substr(0, colon)) * 60) + std::stoi(hhmm.substr(colon + 1));
    }
    catch (...) {
        return -1;
    }
}

int TicketService::ElapsedMinutes(int startMinutes, int endMinutes) {
    // 计算行程、价格或时间等业务数据。
    if (startMinutes < 0 || endMinutes < 0)
        return -1;
    // 持续处理，直到满足结束条件。
    while (endMinutes < startMinutes) {
        endMinutes += 24 * 60;
    }

    return endMinutes - startMinutes;
}

std::string TicketService::FormatDurationMinutes(int minutes) {
    // 组合和转换字段，生成程序或前端使用的数据。
    if (minutes <= 0)
        return "0min";
    const int hours = minutes / 60;
    const int mins = minutes % 60;
    // 发现异常或无效结果时，及时结束当前分支。
    if (hours <= 0) return std::to_string(mins)
        + "min";
    // 根据当前条件决定是否进入下一步处理。
    if (mins == 0) return std::to_string(hours)
        + "h";
    return std::to_string(hours) + "h" + std::to_string(mins) + "min";
}

const TicketService::SeatType* TicketService::CheapestSeatType(const Train& train) {
    // 计算行程、价格或时间等业务数据。
    if (train.seatTypes.empty())
        return nullptr;
    return &(*std::min_element(train.seatTypes.begin(), train.seatTypes.end(), [](const SeatType& left, const SeatType& right) {
        return left.price < right.price;
    }));
}

const TicketService::SeatType* TicketService::FindSeatType(const Train& train, const std::string& seatType) const {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    const std::string decodedSeatType = DecodeJsonEscape(seatType);
    auto it = std::find_if(train.seatTypes.begin(), train.seatTypes.end(), [&](const SeatType& seat) {
        return seat.type == decodedSeatType || seat.type == seatType;
    });
    // 先确认目标数据存在，避免访问无效内容。
    if (it != train.seatTypes.end())
        return &(*it);
    return CheapestSeatType(train);
}
const TicketService::Train* TicketService::FindTrain(const std::string& trainNo) const {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    const auto it = std::find_if(trains_.begin(), trains_.end(), [&](const Train& train) { return train.trainNo == trainNo; });
    return it == trains_.end() ? nullptr : &(*it);
}

TicketService::Train* TicketService::FindTrain(const std::string& trainNo) {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    const auto it = std::find_if(trains_.begin(), trains_.end(), [&](const Train& train) { return train.trainNo == trainNo; });
    return it == trains_.end() ? nullptr : &(*it);
}
bool TicketService::StationMatches(const std::string& codeOrName, const std::string& stationName) const {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    if (codeOrName == stationName)
        return true;
    return std::any_of(stations_.begin(), stations_.end(), [&](const Station& station) {
        return station.code == codeOrName && station.name == stationName;
    });
}

