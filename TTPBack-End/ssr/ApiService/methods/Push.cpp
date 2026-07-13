#include "../ApiService.h"

ApiService::HttpResponse ApiService::HandlePush(const HttpRequest& request)
{
    (void)request;
    return NotFound("Push route not found");
}
