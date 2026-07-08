#pragma once

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

class TicketService
{
public:
    TicketService();

    std::string GetStationsJson() const;
    std::string GetAllTrainsJson() const;
    std::string SearchTrainsJson(const std::string& from, const std::string& to, const std::string& date) const;
    std::string GetTrainDetailJson(const std::string& trainNo) const;
    std::string GetTransferRoutesJson(const std::string& from, const std::string& to, const std::string& date) const;
    std::string GetTicketsJson(const std::string& from, const std::string& to, const std::string& date) const;

    std::string LoginJson(const std::string& requestBody);
    std::string GetMeJson() const;
    std::string SaveProfileJson(const std::string& requestBody);
    std::string RegisterJson(const std::string& requestBody);
    std::string CreateOrderJson(const std::string& requestBody, const std::string& usernameOverride = "");
    std::string PayOrderJson(const std::string& orderId);
    std::string GetOrdersJson(const std::string& username, const std::string& status, const std::string& date) const;
    std::string GetRefundFeeJson(const std::string& orderId) const;
    std::string RefundOrderJson(const std::string& orderId, const std::string& requestBody);
    std::string GetChangeOptionsJson(const std::string& orderId, const std::string& date) const;
    std::string ChangeOrderJson(const std::string& orderId, const std::string& requestBody);

    std::string SaveStationJson(const std::string& requestBody, const std::string& editingCode);
    std::string DeleteStationJson(const std::string& code);
    std::string SaveTrainJson(const std::string& requestBody, const std::string& editingTrainNo);
    std::string DeleteTrainJson(const std::string& trainNo);

    static std::string EscapeJson(const std::string& value);

private:
    struct Station
    {
        std::string code;
        std::string name;
        std::string city;
    };

    struct SeatType
    {
        std::string type;
        double price = 0;
        int remain = 0;
    };

    struct Stop
    {
        std::string station;
        std::string arrive;
        std::string depart;
        int seq = 0;
        int distanceKm = 0;
    };

    struct Train
    {
        std::string trainNo;
        std::string fromStation;
        std::string toStation;
        std::string departureTime;
        std::string arrivalTime;
        std::string duration;
        std::vector<SeatType> seatTypes;
        std::vector<Stop> stops;
    };

    struct Order
    {
        std::string orderId;
        std::string username;
        std::string trainNo;
        std::string date;
        std::string fromStation;
        std::string toStation;
        std::string seatType;
        double totalPrice = 0;
        std::string realName;
        std::string idCard;
        std::string seatNo;
        std::string status;
        std::string createdAt;
        std::string expireAt;
    };

    struct UserProfile
    {
        std::string username = "user";
        std::string role = "user";
        std::string realName = "";
        std::string idCard = "";
        std::string phone = "";
    };

    struct UserAccount
    {
        std::string username;
        std::string phone;
        std::string password;
        std::string role = "user";
        std::string realName = "";
        std::string idCard = "";
    };

    struct SegmentInfo
    {
        std::size_t fromIndex = 0;
        std::size_t toIndex = 0;
        int distanceKm = 0;
    };

    mutable std::mutex mutex_;
    std::vector<Station> stations_;
    std::vector<Train> trains_;
    std::vector<Order> orders_;
    std::vector<UserAccount> users_;
    UserProfile profile_;
    int nextOrderId_ = 1000;

    void LoadData();
    void SaveData() const;
    void SeedDefaultData();

    static std::filesystem::path DataPath(const std::string& fileName);
    static std::vector<std::string> Split(const std::string& value, char delimiter);
    static std::string JoinSeatTypes(const std::vector<SeatType>& seatTypes);
    static std::string JoinStops(const std::vector<Stop>& stops);
    static std::vector<SeatType> ParseSeatTypes(const std::string& value);
    static std::vector<Stop> ParseStops(const std::string& value);
    static std::string OrderToJson(const Order& order, bool brief = false);
    static std::vector<SeatType> ParseSeatTypesFromJson(const std::string& json);
    static std::vector<Stop> ParseStopsFromJson(const std::string& json);
    static int ExtractInt(const std::string& json, const std::string& key, int fallback = 0);
    static double ExtractDouble(const std::string& json, const std::string& key, double fallback = 0);
    static int PassengerCountFromOrderJson(const std::string& json);

    static std::string ExtractString(const std::string& json, const std::string& key);
    static std::string DecodeJsonEscape(const std::string& value);
    static std::string DecodeLooseUnicodeEscapes(const std::string& value);
    static void AppendUtf8(std::ostringstream& output, unsigned int codepoint);
    static std::string ApiResponse(const std::string& dataJson, int code = 0, const std::string& message = "success");
    std::string StationToJson(const Station& station) const;
    static int TotalDistanceKm(const Train& train);
    static double TripPrice(const SeatType& seat, int totalDistanceKm);
    static std::string TrainToJson(const Train& train, const std::string& date, bool includeStops);
    static std::string TrainToAdminListJson(const Train& train, const std::string& date);
    std::string ResolveStationName(const std::string& codeOrName) const;
    bool TryGetSegment(const Train& train, const std::string& from, const std::string& to, SegmentInfo& segment) const;
    bool TryGetQuerySegment(const Train& train, const std::string& from, const std::string& to, SegmentInfo& segment) const;
    std::string BuildSegmentTrainJson(const Train& train, const SegmentInfo& segment, const std::string& date) const;
    std::string BuildTransferDirectJson(const Train& train, const SegmentInfo& segment) const;
    std::string BuildTransferRouteJson(const Train& firstTrain, const SegmentInfo& firstSegment, const Train& secondTrain, const SegmentInfo& secondSegment) const;
    static std::string SeatTypesForSegmentJson(const Train& train, int distanceKm);
    static int SegmentDistanceKm(const Train& train, std::size_t fromIndex, std::size_t toIndex);
    static std::string SegmentDuration(const Train& train, std::size_t fromIndex, std::size_t toIndex);
    static int ParseClockMinutes(const std::string& hhmm);
    static int ElapsedMinutes(int startMinutes, int endMinutes);
    static std::string FormatDurationMinutes(int minutes);
    static const SeatType* CheapestSeatType(const Train& train);
    const SeatType* FindSeatType(const Train& train, const std::string& seatType) const;
    const UserAccount* FindUser(const std::string& usernameOrPhone) const;
    UserAccount* FindUser(const std::string& usernameOrPhone);
    void EnsureDefaultUsers();
    void SyncProfileFromUser(const UserAccount& user);
    const Train* FindTrain(const std::string& trainNo) const;
    Train* FindTrain(const std::string& trainNo);
    Order* FindOrder(const std::string& orderId);
    bool StationMatches(const std::string& codeOrName, const std::string& stationName) const;
};





