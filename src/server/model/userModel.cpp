#include "userModel.hpp"
#include "connection.hpp"
#include "connectionPool.hpp"

bool UserModel::insert(User &user) {
  // 组装sql语句
  char sql[1024] = {0};
  snprintf(sql, sizeof(sql),
           "insert into user(name, password, state) values('%s', '%s', '%s')",
           user.getName().c_str(), user.getPwd().c_str(),
           user.getState().c_str());

  ConnectionPool *pcp = ConnectionPool::getConnectionPool();
  shared_ptr<Connection> pconn = pcp->getConnection();
  if (pconn != nullptr) {
    if (pconn->update(sql)) {
      user.setId(mysql_insert_id(pconn->getConnection()));
      return true;
    }
  }

  // Connection mysql;
  // if (mysql.connect()) {
  //   if (mysql.update(sql)) {
  //     user.setId(mysql_insert_id(mysql.getConnection()));
  //     return true;
  //   }
  // }

  return false;
}

// 根据用户号码查询用户信息
User UserModel::query(int id) {
  // 组装sql语句
  char sql[1024] = {0};
  snprintf(sql, sizeof(sql), "select * from user where id = %d", id);

  ConnectionPool *pcp = ConnectionPool::getConnectionPool();
  shared_ptr<Connection> pconn = pcp->getConnection();
  if (pconn != nullptr) {
    MYSQL_RES *res = pconn->query(sql);
    if (res != nullptr) {
      MYSQL_ROW row = mysql_fetch_row(res);
      if (row != nullptr) {
        // 生成一个User对象，填入信息
        User user;
        user.setId(atoi(row[0]));
        user.setName(row[1]);
        user.setPwd(row[2]);
        user.setState(row[3]);
        // 释放资源
        mysql_free_result(res);
        return user;
      }
    }
  }

  // Connection mysql;
  // if (mysql.connect()) {
  //   MYSQL_RES *res = mysql.query(sql);
  //   if (res != nullptr) {
  //     MYSQL_ROW row = mysql_fetch_row(res);
  //     if (row != nullptr) {
  //       // 生成一个User对象，填入信息
  //       User user;
  //       user.setId(atoi(row[0]));
  //       user.setName(row[1]);
  //       user.setPwd(row[2]);
  //       user.setState(row[3]);
  //       // 释放资源
  //       mysql_free_result(res);
  //       return user;
  //     }
  //   }
  // }

  // 返回空User
  return User();
}

bool UserModel::updateState(User user) {
  char sql[1024] = {0};
  snprintf(sql, sizeof(sql), "update user set state = '%s' where id =%d",
           user.getState().c_str(), user.getId());

  ConnectionPool *pcp = ConnectionPool::getConnectionPool();
  shared_ptr<Connection> pconn = pcp->getConnection();
  if (pconn != nullptr) {
    if (pconn->update(sql)) {
      return true;
    }
  }

  // Connection mysql;
  // if (mysql.connect()) {
  //   if (mysql.update(sql)) {
  //     return true;
  //   }
  // }

  return false;
}

// 重置用户的状态信息
void UserModel::resetState() {
  // 1.组装sql语句
  char sql[1024] = "update user set state = 'offline' where state = 'online'";

  ConnectionPool *pcp = ConnectionPool::getConnectionPool();
  shared_ptr<Connection> pconn = pcp->getConnection();
  if (pconn != nullptr) {
    pconn->update(sql);
  }

  // Connection mysql;
  // if (mysql.connect()) {
  //   mysql.update(sql);
  // }
}