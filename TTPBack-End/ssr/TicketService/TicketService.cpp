#include "TicketService.h"
#include "../Logger.h"

#include <algorithm>
#include <fstream>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <tuple>

TicketService::TicketService()
{
    SeedDefaultData();
    LoadData();
    SaveData();
}

void TicketService::SeedDefaultData()
{
    stations_ = {
        {"BJP", "Beijing", "Beijing"},
        {"SHH", "Shanghai", "Shanghai"},
        {"NJH", "Nanjing", "Nanjing"},
        {"HZH", "Hangzhou", "Hangzhou"},
        {"GZQ", "Guangzhou", "Guangzhou"}
    };

    trains_ = {
        {"G101", "Beijing", "Shanghai", "08:00", "12:30", "4h30min",
            {{"Second Class", 0.42, 120}, {"First Class", 0.72, 45}, {"Business", 1.35, 8}},
            {{"Beijing", "-", "08:00", 1, 0}, {"Nanjing", "11:10", "11:13", 2, 1000}, {"Shanghai", "12:30", "-", 3, 300}}},
        {"G202", "Shanghai", "Hangzhou", "13:30", "14:25", "55min",
            {{"Second Class", 0.42, 180}, {"First Class", 0.72, 45}},
            {{"Shanghai", "-", "13:30", 1, 0}, {"Hangzhou", "14:25", "-", 2, 170}}},
        {"G303", "Beijing", "Guangzhou", "09:15", "17:35", "8h20min",
            {{"Second Class", 0.42, 90}, {"First Class", 0.72, 20}},
            {{"Beijing", "-", "09:15", 1, 0}, {"Nanjing", "12:50", "12:56", 2, 1000}, {"Guangzhou", "17:35", "-", 3, 1130}}}
    };

    orders_.clear();
    users_ = {
        {"user", "13900139000", "1111", "user", "Alice", "110101199001011235"},
        {"admin", "", "1111", "admin", "Admin", ""}
    };
    nextOrderId_ = 1000;
}

void TicketService::LoadData()
{
    std::ifstream stationFile(DataPath("stations.csv"));
    if (stationFile)
    {
        stations_.clear();
        std::string line;
        while (std::getline(stationFile, line))
        {
            if (line.empty()) continue;
            const auto fields = Split(line, '|');
            if (fields.size() >= 3) stations_.push_back({fields[0], fields[1], fields[2]});
        }
    }

    std::ifstream trainFile(DataPath("trains.csv"));
    if (trainFile)
    {
        trains_.clear();
        std::string line;
        while (std::getline(trainFile, line))
        {
            if (line.empty()) continue;
            const auto fields = Split(line, '|');
            if (fields.size() >= 8)
            {
                trains_.push_back({fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], ParseSeatTypes(fields[6]), ParseStops(fields[7])});
            }
        }
        for (auto& train : trains_)
        {
            const int distance = TotalDistanceKm(train);
            if (distance <= 0) continue;
            for (auto& seat : train.seatTypes)
            {
                if (seat.price > 10.0)
                {
                    seat.price = seat.price / distance;
                }
            }
        }
    }

    std::ifstream userFile(DataPath("users.csv"));
    if (userFile)
    {
        users_.clear();
        std::string line;
        while (std::getline(userFile, line))
        {
            if (line.empty()) continue;
            const auto fields = Split(line, '|');
            if (fields.size() >= 6)
            {
                users_.push_back({fields[0], fields[1], fields[2], fields[3], fields[4], fields[5]});
            }
            else if (fields.size() >= 5)
            {
                users_.push_back({fields[0], fields[4], "1111", fields[1], fields[2], fields[3]});
            }
        }
    }
    EnsureDefaultUsers();
    if (!users_.empty())
    {
        SyncProfileFromUser(users_.front());
    }

    std::ifstream orderFile(DataPath("orders.csv"));
    if (orderFile)
    {
        orders_.clear();
        std::string line;
        while (std::getline(orderFile, line))
        {
            if (line.empty()) continue;
            const auto fields = Split(line, '|');
            if (!fields.empty() && fields[0].size() >= 12)
            {
                try
                {
                    nextOrderId_ = std::max(nextOrderId_, std::stoi(fields[0].substr(fields[0].size() - 4)) + 1);
                }
                catch (...)
                {
                    Logger::Warn("ignore invalid order id while advancing sequence id=" + fields[0]);
                }
            }
            if (fields.size() >= 13)
            {
                try
                {
                    Order order;
                    if (fields.size() >= 14)
                    {
                        order = {fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], fields[6], std::stod(fields[7]), fields[8], fields[9], fields[10], fields[11], fields[12], fields[13]};
                    }
                    else
                    {
                        order = {fields[0], "user", fields[1], fields[2], fields[3], fields[4], fields[5], std::stod(fields[6]), fields[7], fields[8], fields[9], fields[10], fields[11], fields[12]};
                    }
                    order.fromStation = ResolveStationName(DecodeLooseUnicodeEscapes(order.fromStation));
                    order.toStation = ResolveStationName(DecodeLooseUnicodeEscapes(order.toStation));
                    order.seatType = DecodeLooseUnicodeEscapes(order.seatType);
                    order.realName = DecodeLooseUnicodeEscapes(order.realName);
                    const Train* fallbackTrain = FindTrain(order.trainNo);
                    if (fallbackTrain != nullptr)
                    {
                        if (order.fromStation.find('?') != std::string::npos)
                        {
                            order.fromStation = fallbackTrain->fromStation;
                        }
                        if (order.toStation.find('?') != std::string::npos)
                        {
                            order.toStation = fallbackTrain->toStation;
                        }
                        if (order.seatType.find('?') != std::string::npos)
                        {
                            const SeatType* fallbackSeat = CheapestSeatType(*fallbackTrain);
                            order.seatType = fallbackSeat == nullptr ? order.seatType : fallbackSeat->type;
                        }
                    }
                    orders_.push_back(order);
                    if (order.orderId.size() >= 12)
                    {
                        nextOrderId_ = std::max(nextOrderId_, std::stoi(order.orderId.substr(order.orderId.size() - 4)) + 1);
                    }
                }
                catch (const std::exception& ex)
                {
                    Logger::Warn("skip invalid order row id=" + fields[0] + " reason=\"" + EscapeJson(ex.what()) + "\"");
                }
            }
            else
            {
                Logger::Warn("skip malformed order row fieldCount=" + std::to_string(fields.size()));
            }
        }
    }
}

void TicketService::SaveData() const
{
    std::filesystem::create_directories(DataPath(".").parent_path());

    std::ofstream stationFile(DataPath("stations.csv"), std::ios::trunc);
    for (const auto& station : stations_)
    {
        stationFile << station.code << '|' << station.name << '|' << station.city << '\n';
    }

    std::ofstream trainFile(DataPath("trains.csv"), std::ios::trunc);
    for (const auto& train : trains_)
    {
        trainFile << train.trainNo << '|' << train.fromStation << '|' << train.toStation << '|'
                  << train.departureTime << '|' << train.arrivalTime << '|' << train.duration << '|'
                  << JoinSeatTypes(train.seatTypes) << '|' << JoinStops(train.stops) << '\n';
    }

    std::ofstream userFile(DataPath("users.csv"), std::ios::trunc);
    for (const auto& user : users_)
    {
        userFile << user.username << '|' << user.phone << '|' << user.password << '|'
                 << user.role << '|' << user.realName << '|' << user.idCard << '\n';
    }

    std::ofstream orderFile(DataPath("orders.csv"), std::ios::trunc);
    for (const auto& order : orders_)
    {
        orderFile << order.orderId << '|' << order.username << '|' << order.trainNo << '|' << order.date << '|'
                  << order.fromStation << '|' << order.toStation << '|' << order.seatType << '|'
                  << order.totalPrice << '|' << order.realName << '|' << order.idCard << '|'
                  << order.seatNo << '|' << order.status << '|' << order.createdAt << '|' << order.expireAt << '\n';
    }
}

