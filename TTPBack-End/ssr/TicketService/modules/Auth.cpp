// 文件说明：认证模块：处理用户登录、注册和个人资料保存。

#include "../TicketService.h"

#include <algorithm>
#include <mutex>
#include <sstream>

std::string TicketService::LoginJson(const std::string& requestBody) {
    // 校验账号和密码，登录成功后同步当前用户资料。
    const std::string username = ExtractString(requestBody, "username");
    const std::string password = ExtractString(requestBody, "password");
    // 先确认目标数据存在，避免访问无效内容。
    if (username.empty() || password.empty())
        return ApiResponse("null", 40001, "username and password are required");
    const UserAccount* user = FindUser(username);
    // 先确认目标数据存在，避免访问无效内容。
    if (user == nullptr || user->password != password)
        return ApiResponse("null", 40101, "invalid username or password");
    SyncProfileFromUser(*user);
    const std::string token = "mock-backend-token-" + EscapeJson(user->username);
    const std::string data = "{\"token\":\"" + token + "\",\"username\":\"" + EscapeJson(user->username) + "\",\"role\":\"" + EscapeJson(user->role) + "\"}";
    return "{\"code\":0,\"message\":\"login success\",\"data\":" + data +
           ",\"token\":\"" + token + "\",\"username\":\"" + EscapeJson(user->username) + "\",\"role\":\"" + EscapeJson(user->role) + "\"}";
}

std::string TicketService::GetMeJson() const {
    // 读取所需数据，并整理为接口需要的返回结果。
    std::lock_guard<std::mutex> lock(mutex_);
    return ApiResponse("{\"id\":1,\"username\":\"" + EscapeJson(profile_.username) +
        "\",\"realName\":\"" + EscapeJson(profile_.realName) +
        "\",\"idCard\":\"" + EscapeJson(profile_.idCard) +
        "\",\"phone\":\"" + EscapeJson(profile_.phone) +
        "\",\"role\":\"" + EscapeJson(profile_.role) + "\"}", 0, "ok");
}

std::string TicketService::SaveProfileJson(const std::string& requestBody) {
    // 更新并保存相关数据，保证程序状态正确。
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string realName = ExtractString(requestBody, "realName");
    const std::string idCard = ExtractString(requestBody, "idCard");
    const std::string phone = ExtractString(requestBody, "phone");
    // 先确认目标数据存在，避免访问无效内容。
    if (!realName.empty())
        profile_.realName = realName;
    // 先确认目标数据存在，避免访问无效内容。
    if (!idCard.empty())
        profile_.idCard = idCard;
    profile_.phone = phone;
    SaveData();
    return ApiResponse("null", 0, "profile updated");
}
std::string TicketService::RegisterJson(const std::string& requestBody) {
    // 注册前检查账号和手机号是否已存在，再创建普通用户。
    const std::string phone = ExtractString(requestBody, "phone");
    const std::string code = ExtractString(requestBody, "code");
    const std::string password = ExtractString(requestBody, "password");

    // 先确认目标数据存在，避免访问无效内容。
    if (phone.empty() || password.empty())
        return ApiResponse("null", 40001, "phone and password are required");
    // 根据当前条件决定是否进入下一步处理。
    if (code != "888888")
        return ApiResponse("null", 40001, "verification code must be 888888");
    // 先确认目标数据存在，避免访问无效内容。
    if (FindUser(phone) != nullptr)
        return ApiResponse("null", 40002, "user already exists");
    users_.push_back({phone, phone, password, "user", "", ""});
    SyncProfileFromUser(users_.back());
    SaveData();

    const std::string token = "mock-register-token-" + EscapeJson(phone);
    const std::string data = "{\"token\":\"" + token + "\",\"phone\":\"" + EscapeJson(phone) + "\"}";
    return "{\"code\":0,\"message\":\"register success\",\"data\":" + data +
           ",\"token\":\"" + token + "\",\"phone\":\"" + EscapeJson(phone) + "\"}";
}
const TicketService::UserAccount* TicketService::FindUser(const std::string& usernameOrPhone) const {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    const auto it = std::find_if(users_.begin(), users_.end(), [&](const UserAccount& user) {
        return user.username == usernameOrPhone || user.phone == usernameOrPhone;
    });
    return it == users_.end() ? nullptr : &(*it);
}

TicketService::UserAccount* TicketService::FindUser(const std::string& usernameOrPhone) {
    // 在现有数据中查找并判断是否存在符合条件的记录。
    const auto it = std::find_if(users_.begin(), users_.end(), [&](const UserAccount& user) {
        return user.username == usernameOrPhone || user.phone == usernameOrPhone;
    });
    return it == users_.end() ? nullptr : &(*it);
}

void TicketService::EnsureDefaultUsers() {
    // 更新并保存相关数据，保证程序状态正确。
    if (FindUser("user") == nullptr) {
        users_.push_back({"user", "13900139000", "1111", "user", "Alice", "110101199001011235"});
    }
    // 先确认目标数据存在，避免访问无效内容。
    if (FindUser("admin") == nullptr) {
        users_.push_back({"admin", "", "1111", "admin", "Admin", ""});
    }
}

void TicketService::SyncProfileFromUser(const UserAccount& user) {
    // 更新并保存相关数据，保证程序状态正确。
    profile_ = {user.username, user.role, user.realName, user.idCard, user.phone};
}

