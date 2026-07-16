// 文件说明：JSON 工具模块：负责字段解析、转义和接口数据拼接。

#include "../TicketService.h"

#include <cctype>
#include <filesystem>
#include <iomanip>
#include <sstream>

std::string TicketService::EscapeJson(const std::string& value) {
    // 转义特殊字符，避免文本内容破坏 JSON 格式。
    std::ostringstream escaped;
    // 依次处理集合中的每一项数据。
    for (const char ch : value) {
        switch (ch) {
        case '\\': escaped << "\\\\"; break;
        case '"': escaped << "\\\""; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default: escaped << ch; break;
        }
    }
    return escaped.str();
}

std::filesystem::path TicketService::DataPath(const std::string& fileName) {
    // 执行该函数对应的处理，并返回结果。
    const std::filesystem::path newBackendData = std::filesystem::path("Back-end") / "data" / fileName;
    // 根据当前条件决定是否进入下一步处理。
    if (std::filesystem::exists("Back-end"))
        return newBackendData;
    const std::filesystem::path oldBackendData = std::filesystem::path("TranTicketBack-end") / "data" / fileName;
    // 根据当前条件决定是否进入下一步处理。
    if (std::filesystem::exists("TranTicketBack-end"))
        return oldBackendData;
    return std::filesystem::path("data") / fileName;
}
std::vector<std::string> TicketService::Split(const std::string& value, char delimiter) {
    // 解析输入内容，转换为程序内部可用的数据。
    std::vector<std::string> result;
    std::stringstream stream(value);
    std::string item;
    // 持续处理，直到满足结束条件。
    while (std::getline(stream, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

std::string TicketService::JoinSeatTypes(const std::vector<SeatType>& seatTypes) {
    // 组合和转换字段，生成程序或前端使用的数据。
    std::ostringstream output;
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < seatTypes.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            output << ';';
        output << seatTypes[i].type << ',' << seatTypes[i].price << ',' << seatTypes[i].remain;
    }
    return output.str();
}

std::string TicketService::JoinStops(const std::vector<Stop>& stops) {
    // 组合和转换字段，生成程序或前端使用的数据。
    std::ostringstream output;
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < stops.size(); ++i) {
        // 根据当前条件决定是否进入下一步处理。
        if (i > 0)
            output << ';';
        output << stops[i].station << ',' << stops[i].arrive << ',' << stops[i].depart << ',' << stops[i].seq << ',' << stops[i].distanceKm;
    }
    return output.str();
}

std::vector<TicketService::SeatType> TicketService::ParseSeatTypes(const std::string& value) {
    // 解析输入内容，转换为程序内部可用的数据。
    std::vector<SeatType> result;
    // 依次处理集合中的每一项数据。
    for (const auto& item : Split(value, ';')) {
        const auto fields = Split(item, ',');
        // 检查数据数量或长度是否满足当前处理要求。
        if (fields.size() >= 3)
            result.push_back({fields[0], std::stod(fields[1]), std::stoi(fields[2])});
    }
    return result;
}

std::vector<TicketService::Stop> TicketService::ParseStops(const std::string& value) {
    // 解析输入内容，转换为程序内部可用的数据。
    std::vector<Stop> result;
    auto defaultDistance = [](const std::string& from, const std::string& to) {
        // 根据当前条件决定是否进入下一步处理。
        if ((from == "Beijing" && to == "Nanjing") || (from == "Nanjing" && to == "Beijing"))
            return 1000;
        // 根据当前条件决定是否进入下一步处理。
        if ((from == "Nanjing" && to == "Shanghai") || (from == "Shanghai" && to == "Nanjing"))
            return 300;
        // 根据当前条件决定是否进入下一步处理。
        if ((from == "Shanghai" && to == "Hangzhou") || (from == "Hangzhou" && to == "Shanghai"))
            return 170;
        // 根据当前条件决定是否进入下一步处理。
        if ((from == "Nanjing" && to == "Guangzhou") || (from == "Guangzhou" && to == "Nanjing"))
            return 1130;
        // 根据当前条件决定是否进入下一步处理。
        if ((from == "Beijing" && to == "Shanghai") || (from == "Shanghai" && to == "Beijing"))
            return 1300;
        // 根据当前条件决定是否进入下一步处理。
        if ((from == "Beijing" && to == "Guangzhou") || (from == "Guangzhou" && to == "Beijing"))
            return 2130;
        return 0;
    };

    // 依次处理集合中的每一项数据。
    for (const auto& item : Split(value, ';')) {
        const auto fields = Split(item, ',');
        // 检查数据数量或长度是否满足当前处理要求。
        if (fields.size() >= 4) {
            int distanceKm = 0;
            // 检查数据数量或长度是否满足当前处理要求。
            if (fields.size() >= 5) {
                distanceKm = std::stoi(fields[4]);
            }
            else if (!result.empty()) {
                distanceKm = defaultDistance(result.back().station, fields[0]);
            }
            result.push_back({fields[0], fields[1], fields[2], std::stoi(fields[3]), distanceKm});
        }
    }
    return result;
}

std::string TicketService::OrderToJson(const Order& order, bool brief) {
    // 将订单对象整理为前端可以直接使用的 JSON 数据。
    std::ostringstream json;
    json << "{\"orderId\":\"" << EscapeJson(order.orderId) << "\",\"username\":\"" << EscapeJson(order.username) << "\",\"trainNo\":\"" << EscapeJson(order.trainNo)
         << "\",\"date\":\"" << EscapeJson(order.date) << "\",\"fromStation\":\"" << EscapeJson(order.fromStation)
         << "\",\"toStation\":\"" << EscapeJson(order.toStation) << "\"";
    // 根据当前条件决定是否进入下一步处理。
    if (brief) {
        json << ",\"departureTime\":\"08:00\"";
    }
    json << ",\"seatType\":\"" << EscapeJson(order.seatType) << "\",\"totalPrice\":" << order.totalPrice;
    // 根据当前条件决定是否进入下一步处理。
    if (!brief) {
        json << ",\"passengers\":[{\"realName\":\"" << EscapeJson(order.realName)
             << "\",\"idCard\":\"" << EscapeJson(order.idCard)
             << "\",\"seatNo\":\"" << EscapeJson(order.seatNo) << "\"}]";
    }
    json << ",\"status\":\"" << EscapeJson(order.status)
         << "\",\"createdAt\":\"" << EscapeJson(order.createdAt) << "\"";
    // 根据当前条件决定是否进入下一步处理。
    if (!brief) {
        json << ",\"expireAt\":\"" << EscapeJson(order.expireAt) << "\"";
    }
    json << '}';
    return json.str();
}
std::vector<TicketService::SeatType> TicketService::ParseSeatTypesFromJson(const std::string& json) {
    // 解析输入内容，转换为程序内部可用的数据。
    std::vector<SeatType> result;
    const std::string key = "\"seatTypes\"";
    const auto keyPos = json.find(key);
    // 根据当前条件决定是否进入下一步处理。
    if (keyPos == std::string::npos)
        return result;
    const auto arrayStart = json.find('[', keyPos);
    const auto arrayEnd = json.find(']', arrayStart);
    // 根据当前条件决定是否进入下一步处理。
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos)
        return result;
    std::size_t pos = arrayStart;
    // 持续处理，直到满足结束条件。
    while (pos < arrayEnd) {
        const auto objectStart = json.find('{', pos);
        // 根据当前条件决定是否进入下一步处理。
        if (objectStart == std::string::npos || objectStart > arrayEnd)
            break;
        const auto objectEnd = json.find('}', objectStart);
        // 根据当前条件决定是否进入下一步处理。
        if (objectEnd == std::string::npos || objectEnd > arrayEnd)
            break;
        const std::string object = json.substr(objectStart, objectEnd - objectStart + 1);
        const std::string type = ExtractString(object, "type");
        // 先确认目标数据存在，避免访问无效内容。
        if (!type.empty()) {
            result.push_back({type, ExtractDouble(object, "price"), ExtractInt(object, "remain")});
        }
        pos = objectEnd + 1;
    }
    return result;
}

std::vector<TicketService::Stop> TicketService::ParseStopsFromJson(const std::string& json) {
    // 解析输入内容，转换为程序内部可用的数据。
    std::vector<Stop> result;
    const std::string key = "\"stops\"";
    const auto keyPos = json.find(key);
    // 根据当前条件决定是否进入下一步处理。
    if (keyPos == std::string::npos)
        return result;
    const auto arrayStart = json.find('[', keyPos);
    const auto arrayEnd = json.find(']', arrayStart);
    // 根据当前条件决定是否进入下一步处理。
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos)
        return result;
    std::size_t pos = arrayStart;
    // 持续处理，直到满足结束条件。
    while (pos < arrayEnd) {
        const auto objectStart = json.find('{', pos);
        // 根据当前条件决定是否进入下一步处理。
        if (objectStart == std::string::npos || objectStart > arrayEnd)
            break;
        const auto objectEnd = json.find('}', objectStart);
        // 根据当前条件决定是否进入下一步处理。
        if (objectEnd == std::string::npos || objectEnd > arrayEnd)
            break;
        const std::string object = json.substr(objectStart, objectEnd - objectStart + 1);
        const std::string station = ExtractString(object, "station");
        // 先确认目标数据存在，避免访问无效内容。
        if (!station.empty()) {
            result.push_back({station, ExtractString(object, "arrive"), ExtractString(object, "depart"), ExtractInt(object, "seq"), ExtractInt(object, "distanceKm", ExtractInt(object, "distance"))});
        }
        pos = objectEnd + 1;
    }
    return result;
}

int TicketService::ExtractInt(const std::string& json, const std::string& key, int fallback) {
    // 解析输入内容，转换为程序内部可用的数据。
    return static_cast<int>(ExtractDouble(json, key, fallback));
}

double TicketService::ExtractDouble(const std::string& json, const std::string& key, double fallback) {
    // 解析输入内容，转换为程序内部可用的数据。
    const std::string quotedKey = "\"" + key + "\"";
    const auto keyPos = json.find(quotedKey);
    // 根据当前条件决定是否进入下一步处理。
    if (keyPos == std::string::npos)
        return fallback;
    const auto colonPos = json.find(':', keyPos);
    // 根据当前条件决定是否进入下一步处理。
    if (colonPos == std::string::npos)
        return fallback;
    std::size_t valueStart = json.find_first_of("-0123456789", colonPos + 1);
    // 根据当前条件决定是否进入下一步处理。
    if (valueStart == std::string::npos)
        return fallback;
    std::size_t valueEnd = valueStart;
    // 持续处理，直到满足结束条件。
    while (valueEnd < json.size() && (std::isdigit(static_cast<unsigned char>(json[valueEnd])) || json[valueEnd] == '.' || json[valueEnd] == '-')) {
        ++valueEnd;
    }
    try {
        return std::stod(json.substr(valueStart, valueEnd - valueStart));
    }
    catch (...) {
        return fallback;
    }
}
std::vector<std::pair<std::string, std::string>> TicketService::ExtractPassengersFromOrderJson(const std::string& json) {
    // 解析输入内容，转换为程序内部可用的数据。
    std::vector<std::pair<std::string, std::string>> passengers;
    auto keyPos = json.find("\"passengers\"");
    // 根据当前条件决定是否进入下一步处理。
    if (keyPos == std::string::npos)
        keyPos = json.find("\"Passengers\"");
    // 根据当前条件决定是否进入下一步处理。
    if (keyPos == std::string::npos)
        return passengers;
    const auto arrayStart = json.find('[', keyPos);
    // 根据当前条件决定是否进入下一步处理。
    if (arrayStart == std::string::npos)
        return passengers;
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    std::size_t objectStart = std::string::npos;
    // 依次处理集合中的每一项数据。
    for (std::size_t i = arrayStart; i < json.size(); ++i) {
        const char ch = json[i];
        // 根据当前条件决定是否进入下一步处理。
        if (inString) {
            // 根据当前条件决定是否进入下一步处理。
            if (escaped)
                escaped = false;
            else if (ch == '\\')
                escaped = true;
            else if (ch == '"')
                inString = false;
            continue;
        }
        // 根据当前条件决定是否进入下一步处理。
        if (ch == '"') { inString = true; continue; }
        // 根据当前条件决定是否进入下一步处理。
        if (ch == '[') { ++depth; continue; }
        // 根据当前条件决定是否进入下一步处理。
        if (ch == ']') { --depth; if (depth == 0) break; continue; }
        // 根据当前条件决定是否进入下一步处理。
        if (ch == '{' && depth == 1)
            objectStart = i;
        // 根据当前条件决定是否进入下一步处理。
        if (ch == '}' && depth == 1 && objectStart != std::string::npos)
        {
            const std::string object = json.substr(objectStart, i - objectStart + 1);
            std::string realName = DecodeLooseUnicodeEscapes(ExtractString(object, "realName"));
            // 先确认目标数据存在，避免访问无效内容。
            if (realName.empty())
                realName = DecodeLooseUnicodeEscapes(ExtractString(object, "RealName"));
            std::string idCard = ExtractString(object, "idCard");
            // 先确认目标数据存在，避免访问无效内容。
            if (idCard.empty())
                idCard = ExtractString(object, "IdCard");
            passengers.push_back({realName, idCard});
            objectStart = std::string::npos;
        }
    }
    return passengers;
}

int TicketService::PassengerCountFromOrderJson(const std::string& json) {
    // 执行该函数对应的处理，并返回结果。
    auto findKey = [&](const std::string& key) {
        const std::string quotedKey = "\"" + key + "\"";
        return json.find(quotedKey);
    };

    auto keyPos = findKey("passengers");
    // 根据当前条件决定是否进入下一步处理。
    if (keyPos == std::string::npos)
        keyPos = findKey("Passengers");
    // 根据当前条件决定是否进入下一步处理。
    if (keyPos == std::string::npos)
        return 1;
    const auto arrayStart = json.find('[', keyPos);
    // 根据当前条件决定是否进入下一步处理。
    if (arrayStart == std::string::npos)
        return 1;
    int depth = 0;
    int objectCount = 0;
    bool inString = false;
    bool escaped = false;
    // 依次处理集合中的每一项数据。
    for (std::size_t i = arrayStart; i < json.size(); ++i) {
        const char ch = json[i];
        // 根据当前条件决定是否进入下一步处理。
        if (inString) {
            // 根据当前条件决定是否进入下一步处理。
            if (escaped)
                escaped = false;
            else if (ch == '\\')
                escaped = true;
            else if (ch == '"')
                inString = false;
            continue;
        }

        // 根据当前条件决定是否进入下一步处理。
        if (ch == '"') {
            inString = true;
            continue;
        }
        // 根据当前条件决定是否进入下一步处理。
        if (ch == '[') {
            ++depth;
            continue;
        }
        // 根据当前条件决定是否进入下一步处理。
        if (ch == ']') {
            --depth;
            // 根据当前条件决定是否进入下一步处理。
            if (depth == 0)
                break;
            continue;
        }
        // 根据当前条件决定是否进入下一步处理。
        if (ch == '{' && depth == 1)
        {
            ++objectCount;
        }
    }

    return objectCount > 0 ? objectCount : 1;
}
std::string TicketService::ExtractString(const std::string& json, const std::string& key) {
    // 解析输入内容，转换为程序内部可用的数据。
    const std::string quotedKey = "\"" + key + "\"";
    auto keyPos = json.find(quotedKey);
    // 先确认目标数据存在，避免访问无效内容。
    if (keyPos == std::string::npos && !key.empty()) {
        std::string altKey = key;
        altKey[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(altKey[0])));
        keyPos = json.find("\"" + altKey + "\"");
    }
    // 根据当前条件决定是否进入下一步处理。
    if (keyPos == std::string::npos)
        return "";
    const auto colonPos = json.find(':', keyPos);
    // 根据当前条件决定是否进入下一步处理。
    if (colonPos == std::string::npos)
        return "";
    const auto quoteStart = json.find('"', colonPos + 1);
    // 根据当前条件决定是否进入下一步处理。
    if (quoteStart == std::string::npos)
        return "";
    std::string result;
    // 依次处理集合中的每一项数据。
    for (std::size_t i = quoteStart + 1; i < json.size(); ++i) {
        // 检查数据数量或长度是否满足当前处理要求。
        if (json[i] == '\\' && i + 1 < json.size()) {
            result.push_back(json[++i]);
            continue;
        }
        // 根据当前条件决定是否进入下一步处理。
        if (json[i] == '"')
            break;
        result.push_back(json[i]);
    }
    return DecodeJsonEscape(result);
}

std::string TicketService::DecodeJsonEscape(const std::string& value) {
    // 解析输入内容，转换为程序内部可用的数据。
    std::ostringstream decoded;
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < value.size(); ++i) {
        // 检查数据数量或长度是否满足当前处理要求。
        if (value[i] != '\\' || i + 1 >= value.size()) {
            decoded << value[i];
            continue;
        }

        const char esc = value[++i];
        switch (esc) {
        case 'n': decoded << '\n'; break;
        case 'r': decoded << '\r'; break;
        case 't': decoded << '\t'; break;
        case '\\': decoded << '\\'; break;
        case '"': decoded << '"'; break;
        case 'u':
            // 检查数据数量或长度是否满足当前处理要求。
            if (i + 4 < value.size()) {
                try {
                    const unsigned int codepoint = static_cast<unsigned int>(std::stoul(value.substr(i + 1, 4), nullptr, 16));
                    AppendUtf8(decoded, codepoint);
                    i += 4;
                    break;
                }
                catch (...) {
                }
            }
            decoded << "\\u";
            break;
        default:
            decoded << esc;
            break;
        }
    }
    return decoded.str();
}


std::string TicketService::DecodeLooseUnicodeEscapes(const std::string& value) {
    // 解析输入内容，转换为程序内部可用的数据。
    std::ostringstream decoded;
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < value.size(); ++i) {
        // 检查数据数量或长度是否满足当前处理要求。
        if (value[i] == 'u' && i + 4 < value.size()) {
            const std::string hex = value.substr(i + 1, 4);
            const bool allHex = std::all_of(hex.begin(), hex.end(), [](unsigned char ch) {
                return std::isxdigit(ch) != 0;
            });
            // 根据当前条件决定是否进入下一步处理。
            if (allHex) {
                try {
                    const unsigned int codepoint = static_cast<unsigned int>(std::stoul(hex, nullptr, 16));
                    AppendUtf8(decoded, codepoint);
                    i += 4;
                    continue;
                }
                catch (...) {
                }
            }
        }
        decoded << value[i];
    }
    return DecodeJsonEscape(decoded.str());
}
void TicketService::AppendUtf8(std::ostringstream& output, unsigned int codepoint) {
    // 组合和转换字段，生成程序或前端使用的数据。
    if (codepoint <= 0x7F) {
        output << static_cast<char>(codepoint);
    }
    else if (codepoint <= 0x7FF) {
        output << static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
        output << static_cast<char>(0x80 | (codepoint & 0x3F));
    }
    else {
        output << static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
        output << static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        output << static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

std::string TicketService::ApiResponse(const std::string& dataJson, int code, const std::string& message) {
    // 统一后端返回格式，包含状态码、提示信息和数据主体。
    return "{\"code\":" + std::to_string(code) + ",\"message\":\"" + EscapeJson(message) + "\",\"data\":" + dataJson + "}";
}

