#include "../ApiService.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <string>

namespace {
// 查询参数中的中文会以百分号编码传入；在交给业务层前恢复原始 UTF-8 字节。
std::string UrlDecode(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            char* end = nullptr;
            const long decoded = std::strtol(hex.c_str(), &end, 16);
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

// 仅接受形如 /api/trains/{id} 的单段路径，防止把子路由误当作资源编号。
std::string PathId(const std::string& path, const std::string& prefix) {
    if (path.rfind(prefix, 0) != 0)
        return "";
    const std::string value = path.substr(prefix.size());
    return value.find('/') == std::string::npos ? UrlDecode(value) : "";
}

std::string PathTail(const std::string& path, const std::string& prefix) {
    return path.rfind(prefix, 0) == 0 ? UrlDecode(path.substr(prefix.size())) : "";
}

std::string Param(const std::map<std::string, std::string>& params, const std::string& key) {
    const auto it = params.find(key);
    return it == params.end() ? "" : it->second;
}

std::string HeaderValue(const ApiService::HttpRequest& request, const std::string& key) {
    std::string lowered = key;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    const auto it = request.headers.find(lowered);
    return it == request.headers.end() ? "" : it->second;
}

// 当前项目使用模拟 Token；优先从请求头识别用户，避免信任 query 参数中的用户名。
std::string UsernameFromAuthorization(const ApiService::HttpRequest& request) {
    std::string auth = HeaderValue(request, "authorization");
    const std::string bearerPrefix = "Bearer ";
    if (auth.rfind(bearerPrefix, 0) == 0)
        auth = auth.substr(bearerPrefix.size());
    const std::string loginPrefix = "mock-backend-token-";
    if (auth.rfind(loginPrefix, 0) == 0)
        return auth.substr(loginPrefix.size());
    const std::string registerPrefix = "mock-register-token-";
    if (auth.rfind(registerPrefix, 0) == 0)
        return auth.substr(registerPrefix.size());
    return "";
}
}

// GET 路由只负责参数提取和分发，具体查询与 JSON 组装由 TicketService 完成。
ApiService::HttpResponse ApiService::HandleGet(const HttpRequest& request) {
    if (request.path == "/api/stations")
        return Ok(ticketService_.GetStationsJson());
    if (request.path == "/api/admin/trains")
        return Ok(ticketService_.GetAllTrainsJson());
    if (request.path == "/api/trains")
        return Ok(ticketService_.SearchTrainsJson(Param(request.queryParams, "from"), Param(request.queryParams, "to"), Param(request.queryParams, "date")));
    if (request.path == "/api/tickets")
        return Ok(ticketService_.GetTicketsJson(Param(request.queryParams, "from"), Param(request.queryParams, "to"), Param(request.queryParams, "date")));
    // 保持既有接口不变，内部由 TicketService 使用 Dijkstra 生成换乘方案。
    if (request.path == "/api/tickets/transfer")
        return Ok(ticketService_.GetTransferRoutesJson(Param(request.queryParams, "from"), Param(request.queryParams, "to"), Param(request.queryParams, "date")));

    const std::string trainNo = PathId(request.path, "/api/trains/");
    if (!trainNo.empty())
        return Ok(ticketService_.GetTrainDetailJson(trainNo));
    if (request.path == "/api/auth/me")
        return Ok(ticketService_.GetMeJson());
    // 订单查询优先使用 Token 身份；无 Token 时才兼容旧调用方传入的 username。
    if (request.path == "/api/orders") {
        const std::string tokenUsername = UsernameFromAuthorization(request);
        const std::string requestedUsername = Param(request.queryParams, "username");
        const std::string username = !tokenUsername.empty() ? tokenUsername : requestedUsername;
        return Ok(ticketService_.GetOrdersJson(username, Param(request.queryParams, "status"), Param(request.queryParams, "date")));
    }

    const std::string orderTail = PathTail(request.path, "/api/orders/");
    if (!orderTail.empty()) {
        const std::string refundFeeSuffix = "/refund-fee";
        if (orderTail.size() > refundFeeSuffix.size() && orderTail.compare(orderTail.size() - refundFeeSuffix.size(), refundFeeSuffix.size(), refundFeeSuffix) == 0)
            return Ok(ticketService_.GetRefundFeeJson(orderTail.substr(0, orderTail.size() - refundFeeSuffix.size())));
        const std::string changeOptionsSuffix = "/change-options";
        if (orderTail.size() > changeOptionsSuffix.size() && orderTail.compare(orderTail.size() - changeOptionsSuffix.size(), changeOptionsSuffix.size(), changeOptionsSuffix) == 0)
            return Ok(ticketService_.GetChangeOptionsJson(orderTail.substr(0, orderTail.size() - changeOptionsSuffix.size()), Param(request.queryParams, "date")));
    }

    return NotFound("Route not found");
}