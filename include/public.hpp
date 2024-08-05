#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/
enum EnMsgType : unsigned int {
  LOGIN_MSG = 1,    // 登录消息
  REG_MSG,          // 注册消息
  REGISTER_MSG_ACK, // 注册成功消息
  LOGIN_MSG_ACK,    // 登陆成功消息
  ONE_CHAT_MSG,     // 一对一聊天消息
  ADD_FRIEND_MSG    // 添加好友消息
};


#endif