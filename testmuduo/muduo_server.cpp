#include <functional>
#include <iostream>
#include <muduo/net/Callbacks.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出CharServer的构造函数
4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
5.设置合适的服务端线程数量，muduo会自己分配IO线程和worker线程
*/
class CharServer {
public:
  CharServer(EventLoop *loop,               // 事件循环
             const InetAddress &listenAddr, // IP+Port
             const string &nameArg)         // 服务器名字
      : _server(loop, listenAddr, nameArg), _loop(loop) {
    // 给服务器注册用户连接的创建和断开回调
    _server.setConnectionCallback(
        std::bind(&CharServer::onConnection, this, _1));
    // 给服务器注册用户读写事件回调
    _server.setMessageCallback(
        std::bind(&CharServer::onMessage, this, _1, _2, _3));
    // 设置服务器端的线程数量 1个I/O线程，3个woker线程
    _server.setThreadNum(4);
  }

  void start() { _server.start(); }

private:
  // 处理用户连接的创建和断开
  void onConnection(const TcpConnectionPtr &conn) {
    if (conn->connected()) {
      cout << conn->peerAddress().toIpPort() << "->"
           << conn->localAddress().toIpPort() << "status: online" << endl;
    } else {
      cout << conn->peerAddress().toIpPort() << "->"
           << conn->localAddress().toIpPort() << "status: offline" << endl;
      conn->shutdown(); // close(fd)
    }
  }

  // 处理用户的读写事件
  void onMessage(const TcpConnectionPtr &conn, // 连接
                 Buffer *buffer,               // 缓冲区
                 Timestamp receiveTime)        // 接收数据的时间信息
  {
    string buf = buffer->retrieveAllAsString();
    cout << "recv data: " << buf << " time: " << receiveTime.toString() << endl;
    conn->send(buf);
  }
  TcpServer _server;
  EventLoop *_loop;
};

int main(){
  EventLoop loop;
  InetAddress addr("127.0.0.1", 6000);
  CharServer server(&loop, addr, "chat_server");

  server.start(); // listenfd epoll_ctl=>epoll
  loop.loop();  // epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等

  return 0;
}