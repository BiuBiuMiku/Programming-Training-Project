#include "../ApiService.h"

#include <algorithm>
#include <cctype>
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

std::string PathTail(const std::string& path, const std::string& prefix)
{
    return path.rfind(prefix, 0) == 0 ? UrlDecode(path.substr(prefix.size())) : "";
}

std::string HeaderValue(const ApiService::HttpRequest& request, const std::string& key)
{
    std::string lowered = key;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    const auto it = request.headers.find(lowered);
    return it == request.headers.end() ? "" : it->second;
}

std::string UsernameFromAuthorization(const ApiService::HttpRequest& request)
{
    std::string auth = HeaderValue(request, "authorization");
    const std::string bearerPrefix = "Bearer ";
    if (auth.rfind(bearerPrefix, 0) == 0) auth = auth.substr(bearerPrefix.size());

    const std::string loginPrefix = "mock-backend-token-";
    if (auth.rfind(loginPrefix, 0) == 0) return auth.substr(loginPrefix.size());

    const std::string registerPrefix = "mock-register-token-";
    if (auth.rfind(registerPrefix, 0) == 0) return auth.substr(registerPrefix.size());

    return "";
}
}

ApiService::HttpResponse ApiService::HandlePost(const HttpRequest& request)
{
    if (request.path == "/api/auth/login")
    {
        const std::string result = ticketService_.LoginJson(request.body);
        return result.find("\"code\":0") != std::string::npos ? Ok(result) : HttpResponse{401, "Unauthorized", result};
    }

    if (request.path == "/api/auth/send-code")
    {
        return Ok("{\"code\":0,\"message\":\"verification code is 888888\",\"data\":null}");
    }

    if (request.path == "/api/auth/register")
    {
        const std::string result = ticketService_.RegisterJson(request.body);
        return result.find("\"code\":0") != std::string::npos ? Created(result) : HttpResponse{400, "Bad Request", result};
    }

    if (request.path == "/api/orders")
    {
        return Created(ticketService_.CreateOrderJson(request.body, UsernameFromAuthorization(request)));
    }

    const std::string orderTail = PathTail(request.path, "/api/orders/");
    if (!orderTail.empty())
    {
        const std::string paySuffix = "/pay";
        if (orderTail.size() > paySuffix.size() && orderTail.compare(orderTail.size() - paySuffix.size(), paySuffix.size(), paySuffix) == 0)
        {
            return Ok(ticketService_.PayOrderJson(orderTail.substr(0, orderTail.size() - paySuffix.size())));
        }

        const std::string refundSuffix = "/refund";
        if (orderTail.size() > refundSuffix.size() && orderTail.compare(orderTail.size() - refundSuffix.size(), refundSuffix.size(), refundSuffix) == 0)
        {
            return Ok(ticketService_.RefundOrderJson(orderTail.substr(0, orderTail.size() - refundSuffix.size()), request.body));
        }

        const std::string changeSuffix = "/change";
        if (orderTail.size() > changeSuffix.size() && orderTail.compare(orderTail.size() - changeSuffix.size(), changeSuffix.size(), changeSuffix) == 0)
        {
            return Ok(ticketService_.ChangeOrderJson(orderTail.substr(0, orderTail.size() - changeSuffix.size()), request.body));
        }
    }

    if (request.path == "/api/admin/stations")
    {
        return Created(ticketService_.SaveStationJson(request.body, ""));
    }

    if (request.path == "/api/admin/trains")
    {
        return Created(ticketService_.SaveTrainJson(request.body, ""));
    }

    return NotFound("Route not found");
}
