// 文件说明：PUSH 接口预留模块：为将来的推送功能保留处理入口。

#include "../ApiService.h"

ApiService::HttpResponse ApiService::HandlePush(const HttpRequest& request) {
    // 当前项目未定义 PUSH 接口，保留该入口便于以后扩展。
    (void)request;
    return NotFound("Push route not found");
}
