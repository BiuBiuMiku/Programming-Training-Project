#include "../ApiService.h"

#include <cstdlib>
#include <string>

namespace
{
std::string UrlDecode(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '%' && i + 2 < value.size())
        {
            const std::string hex = value.substr(i + 1, 2);
            char* end = nullptr;
            const long decoded = std::strtol(hex.c_str(), &end, 16);
            if (end == hex.c_str() + 2)
            {
                result.push_back(static_cast<char>(decoded));
                i += 2;
            }
            else
            {
                result.push_back(value[i]);
            }
        }
        else if (value[i] == '+')
        {
            result.push_back(' ');
        }
        else
        {
            result.push_back(value[i]);
        }
    }
    return result;
}

std::string PathId(const std::string& path, const std::string& prefix)
{
    if (path.rfind(prefix, 0) != 0) return "";
    const std::string value = path.substr(prefix.size());
    return value.find('/') == std::string::npos ? UrlDecode(value) : "";
}
}

ApiService::HttpResponse ApiService::HandleDelete(const HttpRequest& request)
{
    const std::string stationCode = PathId(request.path, "/api/admin/stations/");
    if (!stationCode.empty())
    {
        return Ok(ticketService_.DeleteStationJson(stationCode));
    }

    const std::string adminTrainNo = PathId(request.path, "/api/admin/trains/");
    if (!adminTrainNo.empty())
    {
        return Ok(ticketService_.DeleteTrainJson(adminTrainNo));
    }

    return NotFound("Route not found");
}
