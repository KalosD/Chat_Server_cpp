#include "chatserver.hpp"
#include "chatservice.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <muduo/base/Logging.h>

using namespace std;

// 服务器退出后重置用户状态
// 捕获SIGINT的处理函数
void resetHandler(int) {
  LOG_INFO << "capture the SIGINT, will reset state\n";
  ChatService::instance()->reset();
  exit(0);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
    exit(-1);
  }

  // 解析通过命令行参数传递的ip和port
  char *ip = argv[1];
  uint16_t port = atoi(argv[2]);

  signal(SIGINT, resetHandler);

  EventLoop loop;
  InetAddress addr(ip, port);
  ChatServer server(&loop, addr, "chatserver");

  server.start();
  loop.loop();

  return 0;
}