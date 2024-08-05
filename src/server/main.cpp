#include "chatserver.hpp"
#include "chatservice.hpp"
#include <csignal>
#include <muduo/base/Logging.h>

using namespace std;

// 服务器退出后重置用户状态
// 捕获SIGINT的处理函数
void resetHandler(int) {
  LOG_INFO << "capture the SIGINT, will reset state\n";
  ChatService::instance()->reset();
  exit(0);
}

int main() {
  signal(SIGINT, resetHandler);

  EventLoop loop;
  InetAddress addr("127.0.0.1", 6000);
  ChatServer server(&loop, addr, "chatserver");

  server.start();
  loop.loop();

  return 0;
}