#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include <functional>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop) {
  // 注册连接回调
  _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
  // 注册读写事件回调
  _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
  // 设置线程数量
  _server.setThreadNum(4);
}

void ChatServer::start() { _server.start(); }

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn) {
  // 客户端断开连接
  if (!conn->connected()) {
    ChatService::instance()->clientCloseExceptionHandler(conn);
    conn->shutdown();
  }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer,
                           Timestamp time) {
  string buf = buffer->retrieveAllAsString();
  // 数据的反序列化：解码
  json js = json::parse(buf);
  // 目的：完全解耦网络模块和业务模块的代码
  // 通过js["msgid"]获取=>业务handler=>conn js time
  auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
  msgHandler(conn, js, time);
}
