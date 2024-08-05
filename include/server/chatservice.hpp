#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "json.hpp"
#include "offlinemessageModel.hpp"
#include "user_model.hpp"
#include <functional>
#include <muduo/net/TcpServer.h>
#include <mutex>
#include <unordered_map>

using json = nlohmann::json;

using namespace muduo;
using namespace muduo::net;
using namespace std;

// 处理消息的事件回调方法类型
using MsgHandler =
    std::function<void(const TcpConnectionPtr &, json &, Timestamp)>;

class ChatService {
public:
  // 获取单例模式对象的接口函数
  static ChatService *instance();

  // 处理登录业务
  void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 处理注册业务
  void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 一对一聊天业务
  void one2oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
  // 获取消息对应处理器
  MsgHandler getHandler(int);
  // 处理客户端异常退出
  void clientCloseExceptionHandler(const TcpConnectionPtr &conn);
  // 服务端异常终止之后的操作
  void reset();

private:
  ChatService();
  // 存储消息id和其对应的业务处理方法:消息id对应的处理操作
  unordered_map<int, MsgHandler> _msgHandlerMap;
  // 存储在线用户的通信连接
  unordered_map<int, TcpConnectionPtr> _userConnMap;
  // 定义互斥锁:保证_userConnMap的线程安全
  mutex _connMutex;

  // 数据操作类对象
  UserModel _userModel;
  OfflineMsgModel _offlineMsgModel;
};

#endif