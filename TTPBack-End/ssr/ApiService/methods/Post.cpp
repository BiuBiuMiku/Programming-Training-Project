// 文件说明：POST 接口模块：处理登录、注册、下单、支付等提交请求。

#include "../ApiService.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace {
std::string UrlDecode(const std::string& value) {
    // 执行该函数对应的处理，并返回结果。
    std::string result;
    result.reserve(value.size());
    // 依次处理集合中的每一项数据。
    for (std::size_t i = 0; i < value.size(); ++i) {
        // 检查数据数量或长度是否满足当前处理要求。
        if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            char* end = nullptr;
            const long decoded = std::strtol(hex.c_str(), &end, 16);
            // 根据当前条件决定是否进入下一步处理。
            if (end == hex.c_str() + 2) {
                result.push_back(static_cast<char>(decoded));
                i += 2;
            }
            else {
                result.push_back(value[i]);
            }
        }
        else if (value[i] == '+') {
            result.push_back(' ');
        }
        else {
            result.push_back(value[i]);
        }
    }
    return result;
}

std::string PathTail(const std::string& path, const std::string& prefix) {
    // 执行该函数对应的处理，并返回结果。
    return path.rfind(prefix, 0) == 0 ? UrlDecode(path.substr(prefix.size())) : "";
}

std::string HeaderValue(const ApiService::HttpRequest& request, const std::string& key) {
    // 执行该函数对应的处理，并返回结果。
    std::string lowered = key;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    const auto it = request.headers.find(lowered);
    return it == request.headers.end() ? "" : it->second;
}

std::string UsernameFromAuthorization(const ApiService::HttpRequest& request) {
    // 执行该函数对应的处理，并返回结果。
    std::string auth = HeaderValue(request, "authorization");
    const std::string bearerPrefix = "Bearer ";
    // 根据当前条件决定是否进入下一步处理。
    if (auth.rfind(bearerPrefix, 0) == 0)
        auth = auth.substr(bearerPrefix.size());
    const std::string loginPrefix = "mock-backend-token-";
    // 根据当前条件决定是否进入下一步处理。
    if (auth.rfind(loginPrefix, 0) == 0)
        return auth.substr(loginPrefix.size());
    const std::string registerPrefix = "mock-register-token-";
    // 根据当前条件决定是否进入下一步处理。
    if (auth.rfind(registerPrefix, 0) == 0)
        return auth.substr(registerPrefix.size());
    return "";
}
}

ApiService::HttpResponse ApiService::HandlePost(const HttpRequest& request) {
    // 集中处理创建或操作类接口，例如登录、注册、下单和支付。
    if (request.path == "/api/auth/login") {
        const std::string result = ticketService_.LoginJson(request.body);
        return result.find("\"code\":0") != std::string::npos ? Ok(result) : HttpResponse{401, "Unauthorized", result};
    }

    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path == "/api/auth/send-code")
        return Ok("{\"code\":0,\"message\":\"verification code is 888888\",\"data\":null}");
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path == "/api/auth/register") {
        const std::string result = ticketService_.RegisterJson(request.body);
        return result.find("\"code\":0") != std::string::npos ? Created(result) : HttpResponse{400, "Bad Request", result};
    }

    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path == "/api/orders")
        return Created(ticketService_.CreateOrderJson(request.body, UsernameFromAuthorization(request)));
    const std::string orderTail = PathTail(request.path, "/api/orders/");
    // 先确认目标数据存在，避免访问无效内容。
    if (!orderTail.empty()) {
        const std::string paySuffix = "/pay";
        // 检查数据数量或长度是否满足当前处理要求。
        if (orderTail.size() > paySuffix.size() && orderTail.compare(orderTail.size() - paySuffix.size(), paySuffix.size(), paySuffix) == 0)
            return Ok(ticketService_.PayOrderJson(orderTail.substr(0, orderTail.size() - paySuffix.size())));
        const std::string refundSuffix = "/refund";
        // 检查数据数量或长度是否满足当前处理要求。
        if (orderTail.size() > refundSuffix.size() && orderTail.compare(orderTail.size() - refundSuffix.size(), refundSuffix.size(), refundSuffix) == 0)
            return Ok(ticketService_.RefundOrderJson(orderTail.substr(0, orderTail.size() - refundSuffix.size()), request.body));
        const std::string changeSuffix = "/change";
        // 检查数据数量或长度是否满足当前处理要求。
        if (orderTail.size() > changeSuffix.size() && orderTail.compare(orderTail.size() - changeSuffix.size(), changeSuffix.size(), changeSuffix) == 0)
            return Ok(ticketService_.ChangeOrderJson(orderTail.substr(0, orderTail.size() - changeSuffix.size()), request.body));
    }

    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path == "/api/admin/stations")
        return Created(ticketService_.SaveStationJson(request.body, ""));
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path == "/api/admin/trains")
        return Created(ticketService_.SaveTrainJson(request.body, ""));
    return NotFound("Route not found");
}
