// 文件说明：HTTP 服务模块：接收请求、解析报文、分发接口并返回响应。

#include "ApiService.h"
#include "../Logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace {
std::string UrlDecode(const std::string& value) {
    // 将 URL 中的百分号编码还原，便于后续读取中文等参数。
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

std::size_t ContentLengthFromHeaders(const std::string& headerPart) {
    // 执行该函数对应的处理，并返回结果。
    std::istringstream stream(headerPart);
    std::string line;
    // 持续处理，直到满足结束条件。
    while (std::getline(stream, line)) {
        // 先确认目标数据存在，避免访问无效内容。
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        const auto colon = line.find(':');
        // 根据当前条件决定是否进入下一步处理。
        if (colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        // 根据当前条件决定是否进入下一步处理。
        if (key != "content-length")
            continue;
        const auto valueStart = line.find_first_not_of(" \t", colon + 1);
        // 根据当前条件决定是否进入下一步处理。
        if (valueStart == std::string::npos)
            return 0;
        try {
            return static_cast<std::size_t>(std::stoul(line.substr(valueStart)));
        }
        catch (...) {
            return 0;
        }
    }

    return 0;
}

std::map<std::string, std::string> ParseQuery(const std::string& query) {
    // 解析输入内容，转换为程序内部可用的数据。
    std::map<std::string, std::string> params;
    std::stringstream stream(query);
    std::string pair;

    // 持续处理，直到满足结束条件。
    while (std::getline(stream, pair, '&')) {
        const auto equalPos = pair.find('=');
        // 根据当前条件决定是否进入下一步处理。
        if (equalPos == std::string::npos) {
            params[UrlDecode(pair)] = "";
        }
        else {
            params[UrlDecode(pair.substr(0, equalPos))] = UrlDecode(pair.substr(equalPos + 1));
        }
    }

    return params;
}

std::string PathId(const std::string& path, const std::string& prefix) {
    // 执行该函数对应的处理，并返回结果。
    if (path.rfind(prefix, 0) != 0)
        return "";
    const std::string value = path.substr(prefix.size());
    return value.find('/') == std::string::npos ? UrlDecode(value) : "";
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
    if (auth.rfind(bearerPrefix, 0) == 0) {
        auth = auth.substr(bearerPrefix.size());
    }

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
std::string Param(const std::map<std::string, std::string>& params, const std::string& key) {
    // 执行该函数对应的处理，并返回结果。
    const auto it = params.find(key);
    return it == params.end() ? "" : it->second;
}

std::string TrimForLog(const std::string& value, std::size_t maxLength = 300) {
    // 执行该函数对应的处理，并返回结果。
    if (value.size() <= maxLength)
        return value;
    return value.substr(0, maxLength) + "...(truncated)";
}
std::atomic<unsigned long long> g_requestCounter {1};

std::string RouteLabel(const ApiService::HttpRequest& request) {
    // 处理 HTTP 请求或构造对应的响应结果。
    if (request.method == "GET" && request.path == "/api/stations")
        return "stations.list";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "GET" && request.path == "/api/admin/trains")
        return "admin.trains.list";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "GET" && request.path == "/api/trains")
        return "trains.search";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "GET" && request.path == "/api/tickets")
        return "tickets.search";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "GET" && request.path == "/api/tickets/transfer")
        return "tickets.transfer";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "POST" && request.path == "/api/auth/login")
        return "auth.login";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "POST" && request.path == "/api/auth/register")
        return "auth.register";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "GET" && request.path == "/api/auth/me")
        return "auth.me";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "PUT" && request.path == "/api/auth/profile")
        return "auth.profile.save";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "POST" && request.path == "/api/auth/send-code")
        return "auth.send-code";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "GET" && request.path == "/api/orders")
        return "orders.list";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "POST" && request.path == "/api/orders")
        return "orders.create";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path.rfind("/api/orders/", 0) == 0)
        return "orders.action";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path.rfind("/api/admin/stations", 0) == 0)
        return "admin.stations";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path.rfind("/api/admin/trains", 0) == 0)
        return "admin.trains";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path.rfind("/api/trains/", 0) == 0)
        return "trains.detail";
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "OPTIONS")
        return "cors.options";
    return "unmatched";
}

std::string QueryParamsForLog(const std::map<std::string, std::string>& params) {
    // 执行该函数对应的处理，并返回结果。
    if (params.empty())
        return "{}";
    std::ostringstream stream;
    stream << '{';
    bool first = true;
    // 依次处理集合中的每一项数据。
    for (const auto& item : params) {
        // 根据当前条件决定是否进入下一步处理。
        if (!first)
            stream << ", ";
        stream << item.first << '=' << (item.second.empty() ? "<empty>" : item.second);
        first = false;
    }
    stream << '}';
    return stream.str();
}

std::string RedactJsonSecret(const std::string& body, const std::string& key) {
    // 执行该函数对应的处理，并返回结果。
    std::string result = body;
    const std::string needle = "\"" + key + "\"";
    std::size_t keyPos = 0;
    // 持续处理，直到满足结束条件。
    while ((keyPos = result.find(needle, keyPos)) != std::string::npos) {
        const std::size_t colon = result.find(':', keyPos + needle.size());
        // 根据当前条件决定是否进入下一步处理。
        if (colon == std::string::npos)
            break;
        const std::size_t quoteStart = result.find('"', colon + 1);
        // 根据当前条件决定是否进入下一步处理。
        if (quoteStart == std::string::npos)
            break;
        const std::size_t quoteEnd = result.find('"', quoteStart + 1);
        // 根据当前条件决定是否进入下一步处理。
        if (quoteEnd == std::string::npos)
            break;
        result.replace(quoteStart + 1, quoteEnd - quoteStart - 1, "***");
        keyPos = quoteStart + 4;
    }
    return result;
}

std::string BodyForLog(const ApiService::HttpRequest& request) {
    // 执行该函数对应的处理，并返回结果。
    if (request.body.empty())
        return "<empty>";
    std::string body = request.body;
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path.rfind("/api/auth/", 0) == 0 || request.path == "/api/orders") {
        body = RedactJsonSecret(body, "password");
        body = RedactJsonSecret(body, "code");
        body = RedactJsonSecret(body, "idCard");
    }
    return TrimForLog(body, 600);
}

int ExtractAppCode(const std::string& body) {
    // 解析输入内容，转换为程序内部可用的数据。
    const std::string needle = "\"code\":";
    const std::size_t pos = body.find(needle);
    // 根据当前条件决定是否进入下一步处理。
    if (pos == std::string::npos)
        return 0;
    std::size_t start = pos + needle.size();
    // 持续处理，直到满足结束条件。
    while (start < body.size() && std::isspace(static_cast<unsigned char>(body[start])))
        ++start;
    std::size_t end = start;
    // 检查数据数量或长度是否满足当前处理要求。
    if (end < body.size() && body[end] == '-')
        ++end;
    // 持续处理，直到满足结束条件。
    while (end < body.size() && std::isdigit(static_cast<unsigned char>(body[end])))
        ++end;
    // 根据当前条件决定是否进入下一步处理。
    if (end == start)
        return 0;
    try { return std::stoi(body.substr(start, end - start)); }
    catch (...) { return 0; }
}

std::string ExtractJsonStringField(const std::string& body, const std::string& key) {
    // 解析输入内容，转换为程序内部可用的数据。
    const std::string needle = "\"" + key + "\":";
    const std::size_t pos = body.find(needle);
    // 根据当前条件决定是否进入下一步处理。
    if (pos == std::string::npos)
        return "";
    const std::size_t startQuote = body.find('"', pos + needle.size());
    // 根据当前条件决定是否进入下一步处理。
    if (startQuote == std::string::npos)
        return "";
    const std::size_t endQuote = body.find('"', startQuote + 1);
    // 根据当前条件决定是否进入下一步处理。
    if (endQuote == std::string::npos)
        return "";
    return body.substr(startQuote + 1, endQuote - startQuote - 1);
}

std::size_t CountOccurrences(const std::string& value, const std::string& needle) {
    // 执行该函数对应的处理，并返回结果。
    std::size_t count = 0;
    std::size_t pos = 0;
    // 持续处理，直到满足结束条件。
    while ((pos = value.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

std::string ResponseSummary(const ApiService::HttpRequest& request, const ApiService::HttpResponse& response) {
    // 执行该函数对应的处理，并返回结果。
    std::ostringstream stream;
    stream << "bytes=" << response.body.size()
           << " appCode=" << ExtractAppCode(response.body);

    const std::string message = ExtractJsonStringField(response.body, "message");
    // 先确认目标数据存在，避免访问无效内容。
    if (!message.empty()) stream << " appMessage=\"" << TrimForLog(message, 120)
        << "\"";
    const std::size_t trainCount = CountOccurrences(response.body, "\"trainNo\"");
    const std::size_t orderCount = CountOccurrences(response.body, "\"orderId\"");
    const std::size_t stationCount = CountOccurrences(response.body, "\"code\"");
    // 根据当前条件决定是否进入下一步处理。
    if (trainCount > 0)
        stream << " trainItems=" << trainCount;
    // 根据当前条件决定是否进入下一步处理。
    if (orderCount > 0)
        stream << " orderItems=" << orderCount;
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.path == "/api/stations" && stationCount > 0)
        stream << " stationItems=" << stationCount;
    const bool emptyTrains = response.body.find("\"data\":[]") != std::string::npos || response.body.find("\"trains\":[]") != std::string::npos;
    const bool emptyTransfer = response.body.find("\"directTrains\":[]") != std::string::npos && response.body.find("\"transferRoutes\":[]") != std::string::npos;
    // 根据当前条件决定是否进入下一步处理。
    if (emptyTrains || emptyTransfer)
        stream << " dataState=empty";
    return stream.str();
}


bool HasZeroNumberField(const std::string& body, const std::string& fieldName) {
    // 执行该函数对应的处理，并返回结果。
    const std::string needle = "\"" + fieldName + "\":";
    std::size_t pos = 0;
    // 持续处理，直到满足结束条件。
    while ((pos = body.find(needle, pos)) != std::string::npos) {
        std::size_t start = pos + needle.size();
        // 持续处理，直到满足结束条件。
        while (start < body.size() && std::isspace(static_cast<unsigned char>(body[start])))
            ++start;
        char* end = nullptr;
        const double value = std::strtod(body.c_str() + start, &end);
        // 根据当前条件决定是否进入下一步处理。
        if (end != body.c_str() + start && value == 0.0)
            return true;
        pos = start + 1;
    }
    return false;
}
std::string ResponseWarnings(const ApiService::HttpRequest& request, const ApiService::HttpResponse& response) {
    // 执行该函数对应的处理，并返回结果。
    std::ostringstream warnings;
    bool hasWarning = false;
    const auto addWarning = [&](const std::string& value) {
        // 根据当前条件决定是否进入下一步处理。
        if (hasWarning)
            warnings << "; ";
        warnings << value;
        hasWarning = true;
    };

    const int appCode = ExtractAppCode(response.body);
    // 根据订单当前状态决定是否继续处理。
    if (response.statusCode >= 400)
        addWarning("http-error");
    // 根据订单当前状态决定是否继续处理。
    if (response.statusCode < 400 && appCode != 0)
        addWarning("business-code-nonzero");
    const bool isSearch = request.path == "/api/trains" || request.path == "/api/tickets" || request.path == "/api/tickets/transfer";
    const bool hasSearchParam = !Param(request.queryParams, "from").empty() || !Param(request.queryParams, "to").empty();
    const bool emptyTrains = response.body.find("\"data\":[]") != std::string::npos || response.body.find("\"trains\":[]") != std::string::npos;
    const bool emptyTransfer = response.body.find("\"directTrains\":[]") != std::string::npos && response.body.find("\"transferRoutes\":[]") != std::string::npos;
    // 根据当前条件决定是否进入下一步处理。
    if (isSearch && hasSearchParam && (emptyTrains || emptyTransfer))
        addWarning("search-returned-empty-results");
    // 根据当前条件决定是否进入下一步处理。
    if (HasZeroNumberField(response.body, "totalPrice") || HasZeroNumberField(response.body, "price"))
        addWarning("zero-price-in-response");
    // 根据当前条件决定是否进入下一步处理。
    if (response.body.find("\"duration\":\"0") != std::string::npos || response.body.find("\"totalDuration\":\"0") != std::string::npos)
        addWarning("zero-duration-in-response");
    return warnings.str();
}
}

ApiService::ApiService(TicketService& ticketService, std::uint16_t port)
    : ticketService_(ticketService), port_(port) {
    // 执行该函数对应的处理，并返回结果。
}

int ApiService::Start() {
    // 处理 HTTP 请求或构造对应的响应结果。
#ifndef _WIN32
    std::cerr << "This simple server currently targets Windows Winsock." << std::endl;
    return 1;
#else
    WSADATA wsaData {};
    // 根据当前条件决定是否进入下一步处理。
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Logger::Log("WSAStartup failed");
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    const SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // 发现异常或无效结果时，及时结束当前分支。
    if (listenSocket == INVALID_SOCKET) {
        Logger::Log("socket failed");
        std::cerr << "socket failed." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    // 发现异常或无效结果时，及时结束当前分支。
    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        Logger::Log("bind failed on port " + std::to_string(port_));
        std::cerr << "bind failed. Is port " << port_ << " already in use?" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // 发现异常或无效结果时，及时结束当前分支。
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        Logger::Log("listen failed");
        std::cerr << "listen failed." << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // 主循环：依次接收浏览器请求、调用业务层并返回 JSON 结果。
    while (true) {
        const SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        // 发现异常或无效结果时，及时结束当前分支。
        if (clientSocket == INVALID_SOCKET) {
            Logger::Log("accept failed");
            continue;
        }

        std::vector<char> buffer(16384);
        const int bytesReceived = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
        // 根据当前条件决定是否进入下一步处理。
        if (bytesReceived > 0) {
            try {
                std::string rawRequest(buffer.data(), bytesReceived);
                const auto headerEnd = rawRequest.find("\r\n\r\n");
                // 根据当前条件决定是否进入下一步处理。
                if (headerEnd != std::string::npos) {
                    const std::size_t contentLength = ContentLengthFromHeaders(rawRequest.substr(0, headerEnd));
                    const std::size_t expectedSize = headerEnd + 4 + contentLength;
                    // 持续处理，直到满足结束条件。
                    while (contentLength > 0 && rawRequest.size() < expectedSize) {
                        const int moreBytes = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
                        // 发现异常或无效结果时，及时结束当前分支。
                        if (moreBytes <= 0)
                            break;
                        rawRequest.append(buffer.data(), moreBytes);
                    }
                }

                const unsigned long long requestId = g_requestCounter.fetch_add(1);
                const auto startedAt = std::chrono::steady_clock::now();
                const HttpRequest request = ParseRequest(rawRequest);
                LogRequest(request, requestId, rawRequest.size());
                Logger::Info("ROUTE id=" + std::to_string(requestId) + " endpoint=" + RouteLabel(request));
                const HttpResponse response = Route(request);
                const auto finishedAt = std::chrono::steady_clock::now();
                const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finishedAt - startedAt).count();
                LogResponse(request, response, requestId, durationMs);
                const std::string rawResponse = BuildRawResponse(response);
                send(clientSocket, rawResponse.c_str(), static_cast<int>(rawResponse.size()), 0);
            }
            catch (const std::exception& ex) {
                Logger::Error(std::string("REQUEST_EXCEPTION message=\"") + TicketService::EscapeJson(ex.what()) + "\"");
                const std::string rawResponse = BuildRawResponse(BadRequest(ex.what()));
                send(clientSocket, rawResponse.c_str(), static_cast<int>(rawResponse.size()), 0);
            }
        }

        closesocket(clientSocket);
    }
#endif
}

ApiService::HttpRequest ApiService::ParseRequest(const std::string& raw) const {
    // 把原始 HTTP 文本拆成请求方法、路径、请求头和请求体。
    HttpRequest request;

    const auto headerEnd = raw.find("\r\n\r\n");
    const std::string headerPart = raw.substr(0, headerEnd);
    request.body = headerEnd == std::string::npos ? "" : raw.substr(headerEnd + 4);

    std::istringstream stream(headerPart);
    std::string requestLine;
    std::getline(stream, requestLine);
    // 先确认目标数据存在，避免访问无效内容。
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::string headerLine;
    // 持续处理，直到满足结束条件。
    while (std::getline(stream, headerLine)) {
        // 先确认目标数据存在，避免访问无效内容。
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        const auto colon = headerLine.find(':');
        // 根据当前条件决定是否进入下一步处理。
        if (colon == std::string::npos)
            continue;
        std::string key = headerLine.substr(0, colon);
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        const auto valueStart = headerLine.find_first_not_of(" \t", colon + 1);
        request.headers[key] = valueStart == std::string::npos ? "" : headerLine.substr(valueStart);
    }

    std::istringstream lineStream(requestLine);
    std::string target;
    lineStream >> request.method >> target;

    const auto queryPos = target.find('?');
    // 根据当前条件决定是否进入下一步处理。
    if (queryPos == std::string::npos) {
        request.path = target;
    }
    else {
        request.path = target.substr(0, queryPos);
        request.query = target.substr(queryPos + 1);
        request.queryParams = ParseQuery(request.query);
    }

    return request;
}

ApiService::HttpResponse ApiService::Route(const HttpRequest& request) {
    // 根据 HTTP 方法交给对应的接口处理文件。
    if (request.method == "OPTIONS")
        return Ok("{}");
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "GET")
        return HandleGet(request);
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "POST")
        return HandlePost(request);
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "PUT")
        return HandlePut(request);
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "DELETE")
        return HandleDelete(request);
    // 根据请求方法或路径选择对应的处理逻辑。
    if (request.method == "PUSH")
        return HandlePush(request);
    return NotFound("Route not found");
}
ApiService::HttpResponse ApiService::Ok(const std::string& body) const {
    // 处理 HTTP 请求或构造对应的响应结果。
    return {200, "OK", body};
}

ApiService::HttpResponse ApiService::Created(const std::string& body) const {
    // 执行本次业务操作，并同步更新相关数据。
    return {201, "Created", body};
}

ApiService::HttpResponse ApiService::BadRequest(const std::string& message) const {
    // 处理 HTTP 请求或构造对应的响应结果。
    return {400, "Bad Request", "{\"code\":400,\"message\":\"" + TicketService::EscapeJson(message) + "\",\"data\":null}"};
}

ApiService::HttpResponse ApiService::NotFound(const std::string& message) const {
    // 处理 HTTP 请求或构造对应的响应结果。
    return {404, "Not Found", "{\"code\":404,\"message\":\"" + TicketService::EscapeJson(message) + "\",\"data\":null}"};
}

std::string ApiService::BuildRawResponse(const HttpResponse& response) const {
    // 统一补齐响应头，前端可直接接收 JSON，并允许跨域访问。
    std::ostringstream stream;
    stream << "HTTP/1.1 " << response.statusCode << ' ' << response.statusText << "\r\n"
           << "Content-Type: application/json; charset=utf-8\r\n"
           << "Access-Control-Allow-Origin: *\r\n"
           << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
           << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
           << "Connection: close\r\n"
           << "Content-Length: " << response.body.size() << "\r\n\r\n"
           << response.body;

    return stream.str();
}

void ApiService::LogRequest(const HttpRequest& request, unsigned long long requestId, std::size_t rawBytes) const {
    // 处理日志相关信息，便于查看程序运行情况。
    std::ostringstream stream;
    stream << "REQUEST id=" << requestId
           << " method=" << request.method
           << " path=" << request.path
           << " endpoint=" << RouteLabel(request)
           << " query=" << QueryParamsForLog(request.queryParams)
           << " rawBytes=" << rawBytes
           << " body=" << BodyForLog(request);

    Logger::Info(stream.str());
}

void ApiService::LogResponse(const HttpRequest& request, const HttpResponse& response, unsigned long long requestId, long long durationMs) const {
    // 处理日志相关信息，便于查看程序运行情况。
    std::ostringstream stream;
    stream << "RESPONSE id=" << requestId
           << " method=" << request.method
           << " path=" << request.path
           << " endpoint=" << RouteLabel(request)
           << " http=" << response.statusCode << ' ' << response.statusText
           << " durationMs=" << durationMs
           << ' ' << ResponseSummary(request, response)
           << " body=" << TrimForLog(response.body, 900);

    const std::string warnings = ResponseWarnings(request, response);
    // 先确认目标数据存在，避免访问无效内容。
    if (warnings.empty()) {
        Logger::Info(stream.str());
    }
    else {
        Logger::Warn(stream.str() + " warnings=" + warnings);
    }
}

