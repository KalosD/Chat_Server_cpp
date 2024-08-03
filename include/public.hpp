#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/
enum EnMsgType {
  LOGIN_MSG = 1,    // 登录消息
  REG_MSG,          // 注册消息
  REGISTER_MSG_ACK, // 注册成功消息
  LOGIN_MSG_ACK     // 登陆成功消息
};


#endif