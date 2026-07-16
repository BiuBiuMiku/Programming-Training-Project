// 文件说明：PUT 接口模块：处理用户资料、车站和车次的更新请求。

#include "../ApiService.h"

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

std::string PathId(const std::string& path, const std::string& prefix) {
    // 执行该函数对应的处理，并返回结果。
    if (path.rfind(prefix, 0) != 0)
        return "";
    const std::string value = path.substr(prefix.size());
    return value.find('/') == std::string::npos ? UrlDecode(value) : "";
}
}

ApiService::HttpResponse ApiService::HandlePut(const HttpRequest& request) {
    // 处理已有资料的更新，如个人资料、车站和车次。
    if (request.path == "/api/auth/profile")
        return Ok(ticketService_.SaveProfileJson(request.body));
    const std::string stationCode = PathId(request.path, "/api/admin/stations/");
    // 先确认目标数据存在，避免访问无效内容。
    if (!stationCode.empty())
        return Ok(ticketService_.SaveStationJson(request.body, stationCode));
    const std::string adminTrainNo = PathId(request.path, "/api/admin/trains/");
    // 先确认目标数据存在，避免访问无效内容。
    if (!adminTrainNo.empty())
        return Ok(ticketService_.SaveTrainJson(request.body, adminTrainNo));
    return NotFound("Route not found");
}
