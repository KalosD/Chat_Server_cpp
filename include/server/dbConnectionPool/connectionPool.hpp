#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "connection.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

class ConnectionPool {
public:
  // 获取连接池对象实例
  static ConnectionPool *getConnectionPool();
  // 给外部提供接口：从连接池中获取一个可用的空闲连接
  shared_ptr<Connection> getConnection();

private:
  // 单例
  ConnectionPool();
  // 运行在独立的线程中，专门负责生产新连接
  void produceConnectionTask();
  // 扫描超过maxIdleTime时间的空闲连接，进行对于的连接回收
  void scannerConnectionTask();

  queue<Connection *> _connectionQue; // 存储MYSQL连接
  mutex _queueMutex;         // 保证连接队列线程安全的互斥锁
  atomic_int _connectionCnt; // 记录所创建的connecttion连接的总数量
  condition_variable cv; // 设置条件变量用于连接生产线程和消费线程通信
};

#endif