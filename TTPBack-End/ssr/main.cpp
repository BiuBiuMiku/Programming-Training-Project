#include "ApiService/ApiService.h"
#include "Logger.h"

#include <iostream>

int main()
{
    Logger::Initialize();

    TicketService ticketService;
    ApiService apiService(ticketService, 8080);
    Logger::Log("backend process started");
    Logger::Log("listening on http://localhost:8080");

    std::cout << "TrainTicketBackEnd listening on http://localhost:8080" << std::endl;
    std::cout << "Log file: " << Logger::LogFilePath().string() << std::endl;
    return apiService.Start();
}
