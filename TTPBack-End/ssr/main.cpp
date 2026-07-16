// 文件说明：程序入口：初始化日志、业务服务和 HTTP 服务器。

#include "ApiService/ApiService.h"
#include "Logger.h"

#include <iostream>

int main() {
    // 初始化日志后，创建业务服务并启动 HTTP 接口。
    Logger::Initialize();

    TicketService ticketService;
    ApiService apiService(ticketService, 8080);
    Logger::Log("backend process started");
    Logger::Log("listening on http://localhost:8080");

    std::cout << "TrainTicketBackEnd listening on http://localhost:8080" << std::endl;
    std::cout << "Log file: " << Logger::LogFilePath().string() << std::endl;
    return apiService.Start();
}
