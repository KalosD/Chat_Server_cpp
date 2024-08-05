#include "chatservice.hpp"
#include "db.h"
#include "public.hpp"
#include "user.hpp"
#include <functional>
#include <muduo/base/Logging.h>
#include <mutex>

using namespace muduo;
using namespace std;

// 获取单例对象的接口函数
ChatService *ChatService::instance() {
  static ChatService service;
  return &service;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService() {
  _msgHandlerMap.insert(
      {LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {REGISTER_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {ONE_CHAT_MSG, std::bind(&ChatService::one2oneChat, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {ADD_FRIEND_MSG,
       std::bind(&ChatService::addFriendHandler, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {LOGOUT_MSG, std::bind(&ChatService::logoutHandler, this, _1, _2, _3)});

  // 群组业务管理相关事件处理回调注册
  _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup,
                                                     this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
}

// 获取消息对应处理器
MsgHandler ChatService::getHandler(int msgid) {
  // 记录错误日志：msgid没有对应的事件处理回调
  auto it = _msgHandlerMap.find(msgid);
  if (it == _msgHandlerMap.end()) {
    // 返回一个默认的处理器，空操作
    return [=](const TcpConnectionPtr &, json &, Timestamp) {
      LOG_ERROR << "msgid: " << msgid << " can not find handler!";
    };
  } else {
    return _msgHandlerMap[msgid];
  }
}

// 处理登录业务   ORM  业务层操作的都是对象   DAO
/**
 * 登录业务
 * 从json得到用户id
 * 从数据中获取此id的用户，判断此用户的密码是否等于json获取到的密码
 * 判断用户是否重复登录
 * {"msgid":1,"id":13,"password":"123456"}
 * {"errmsg":"this account is using, input another!","errno":2,"msgid":2}
 */
void ChatService::login(const TcpConnectionPtr &conn, json &js,
                        Timestamp time) {
  // LOG_DEBUG << "do login service!";

  int id = js["id"].get<int>();
  std::string password = js["password"];

  User user = _userModel.query(id);
  if (user.getId() == id && user.getPwd() == password) {
    if (user.getState() == "online") {
      // 该用户已经登录，不能重复登录
      json response;
      response["msgid"] = LOGIN_MSG_ACK;
      response["errno"] = 2;
      response["errmsg"] = "this account is online, input another!";
      conn->send(response.dump());
    } else {
      // 登录成功，记录用户连接信息
      // 需要考虑线程安全问题 onMessage会在不同线程中被调用
      {
        // 只需在建立用户连接的时候加锁
        lock_guard<mutex> lock(_connMutex);
        _userConnMap.insert({id, conn});
      }

      // 登录成功，更新用户状态信息 state offline => online
      user.setState("online");
      _userModel.updateState(user);

      json response;
      response["msgid"] = LOGIN_MSG_ACK;
      response["errno"] = 0;
      response["id"] = user.getId();
      response["name"] = user.getName();

      // // id用户登录成功后，向redis订阅channel(id)
      // _redis.subscribe(id);

      // 查询该用户是否有离线消息
      std::vector<std::string> vec = _offlineMsgModel.query(id);
      if (!vec.empty()) {
        response["offlinemsg"] = vec;
        // 读取该用户的离线消息后，将该用户离线消息删除掉
        _offlineMsgModel.remove(id);
      } else {
        LOG_INFO << "无离线消息";
      }

      // 查询该用户好友信息并返回
      std::vector<User> userVec = _friendModel.query(id);
      if (!userVec.empty()) {
        std::vector<std::string> vec;
        for (auto &user : userVec) {
          json js;
          js["id"] = user.getId();
          js["name"] = user.getName();
          js["state"] = user.getState();
          vec.push_back(js.dump());
        }
        response["friends"] = vec;
      }

      conn->send(response.dump());
    }
  } else {
    // 登录失败: 用户不存或密码错误
    json response;
    response["msgid"] = LOGIN_MSG_ACK;
    response["errno"] = 1;
    response["errmsg"] = "accont does not exist or wrong password!";

    conn->send(response.dump());
  }
}

// 处理注册业务 name pwd
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time) {
  string name = js["name"];
  string pwd = js["password"];

  User user;
  user.setName(name);
  user.setPwd(pwd);
  bool state = _userModel.insert(user);
  if (state) {
    // 注册成功
    json response;
    response["msgid"] = REGISTER_MSG_ACK;
    response["errno"] = 0;
    response["id"] = user.getId();
    // json::dump() 将序列化信息转换为std::string
    conn->send(response.dump());
  } else {
    // 注册失败
    json response;
    response["msgid"] = REGISTER_MSG_ACK;
    response["errno"] = 1;
    // 注册已经失败，不需要在json返回id
    conn->send(response.dump());
  }
}

// 一对一聊天业务
void ChatService::one2oneChat(const TcpConnectionPtr &conn, json &js,
                              Timestamp time) {
  int toid = js["toid"].get<int>();
  {
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(toid);
    // 确认是在线状态
    if (it != _userConnMap.end()) {
      // TcpConnection::send() 直接发送消息
      it->second->send(js.dump());
      return;
    }
  }

  // // 用户在其他主机的情况，publish消息到redis
  // User user = _userModel.query(toid);
  // if (user.getState() == "online") {
  //   return;
  // }

  // toId 不在线则存储离线消息
  _offlineMsgModel.insert(toid, js.dump());
}

// 添加朋友业务
void ChatService::addFriendHandler(const TcpConnectionPtr &conn, json &js,
                                   Timestamp time) {
  int userId = js["id"].get<int>();
  int friendId = js["friendid"].get<int>();

  // 存储好友信息
  _friendModel.insert(userId, friendId);
}

// 处理登出业务
void ChatService::logoutHandler(const TcpConnectionPtr &conn, json &js,
                                Timestamp time) {
  int id = js["id"].get<int>();
  {
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(id);
    if (it != _userConnMap.end()) {
      _userConnMap.erase(id);
    }
  }

  // 更新状态信息
  User user = _userModel.query(id);
  user.setState("offline");
  _userModel.updateState(user);
}

// 处理客户端异常退出

void ChatService::clientCloseExceptionHandler(const TcpConnectionPtr &conn) {
  User user;
  // 互斥锁保护
  {
    lock_guard<mutex> lock(_connMutex);
    for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it) {
      if (it->second == conn) {
        // 从map表删除用户的链接信息
        user.setId(it->first);
        _userConnMap.erase(it);
        break;
      }
    }
  }

  // 更新用户的状态信息
  if (user.getId() != -1) {
    user.setState("offline");
    _userModel.updateState(user);
  }
}

// 服务端异常终止之后的操作
void ChatService::reset() {
  // 组装sql语句
  char sql[1024] = "update user set state='offline' where state='online'";

  MySQL mysql;
  if (mysql.connect()) {
    mysql.update(sql);
  }
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js,
                              Timestamp time) {
  int userId = js["id"].get<int>();
  std::string name = js["groupname"];
  std::string desc = js["groupesc"];

  // 存储新创建的群组消息
  Group group(-1, name, desc);
  if (_groupModel.createGroup(group)) {
    // 存储群组创建人信息
    _groupModel.addGroup(userId, group.getId(), "creator");
    json respond;
    respond["msgid"] = CREATE_GROUP_MSG_ACK;
    respond["msg"] = "Successfully created!";
    conn->send(respond.dump());
  }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js,
                           Timestamp time) {
  int userId = js["id"].get<int>();
  int groupId = js["groupid"].get<int>();
  _groupModel.addGroup(userId, groupId, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js,
                            Timestamp time) {
  int userId = js["id"].get<int>();
  int groupId = js["groupid"].get<int>();
  std::vector<int> userIdVec = _groupModel.queryGroupUsers(userId, groupId);

  lock_guard<mutex> lock(_connMutex);
  for (int id : userIdVec) {
    auto it = _userConnMap.find(id);
    if (it != _userConnMap.end()) {
      // 转发群消息
      it->second->send(js.dump());
    } else {
      // 转储离线消息
      _offlineMsgModel.insert(id, js.dump());
    }
  }
}