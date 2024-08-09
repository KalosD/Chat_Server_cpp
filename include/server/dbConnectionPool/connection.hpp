#ifndef CONNECTION_H
#define CONNECTION_H

#include <ctime>
#include <mysql/mysql.h>
#include <string>
using namespace std;

// 数据库操作类
class Connection {
public:
  // 初始化数据库连接
  Connection();
  // 释放数据库连接资源
  ~Connection();
  // 连接数据库
  bool connect(string ip, string user, string password, string dbname,
               unsigned int port);
  // 更新操作
  bool update(string sql);
  // 查询操作
  MYSQL_RES *query(string sql);
  // 获取连接
  MYSQL *getConnection();
  // 刷新一下连接的起始的空闲时间点
  void refreshAliveTime() { _aliveTime = clock(); }
  // 返回存活的时间
  clock_t getAliveeTime() const { return clock() - _aliveTime; }

private:
  MYSQL *_conn;
  clock_t _aliveTime; // 进入空闲状态后的存活时间
};

#endif