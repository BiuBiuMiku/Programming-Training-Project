#pragma once

#include "TicketService/TicketService.h"

#include <cstdint>
#include <map>
#include <string>

class ApiService
{
public:
    ApiService(TicketService& ticketService, std::uint16_t port);

    int Start();

    struct HttpRequest
    {
        std::string method;
        std::string path;
        std::string query;
        std::string body;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> queryParams;
    };

    struct HttpResponse
    {
        int statusCode = 200;
        std::string statusText = "OK";
        std::string body;
    };

private:
    TicketService& ticketService_;
    std::uint16_t port_;

    HttpRequest ParseRequest(const std::string& raw) const;
    HttpResponse Route(const HttpRequest& request);
    HttpResponse HandleGet(const HttpRequest& request);
    HttpResponse HandlePost(const HttpRequest& request);
    HttpResponse HandlePut(const HttpRequest& request);
    HttpResponse HandleDelete(const HttpRequest& request);
    HttpResponse HandlePush(const HttpRequest& request);

    HttpResponse Ok(const std::string& body) const;
    HttpResponse Created(const std::string& body) const;
    HttpResponse BadRequest(const std::string& message) const;
    HttpResponse NotFound(const std::string& message) const;
    std::string BuildRawResponse(const HttpResponse& response) const;
    void LogRequest(const HttpRequest& request, unsigned long long requestId, std::size_t rawBytes) const;
    void LogResponse(const HttpRequest& request, const HttpResponse& response, unsigned long long requestId, long long durationMs) const;
};

