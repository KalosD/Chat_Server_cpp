#include "connectionPool.hpp"
#include "connection.hpp"
#include "mysqlInit.hpp"
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

// 线程安全的懒汉单例模式函数接口
ConnectionPool *ConnectionPool::getConnectionPool() {
  static ConnectionPool pool; // 静态局部变量的初始化是线程安全的
  return &pool;
}

ConnectionPool::ConnectionPool() {
  // 创建初始连接数量
  for (int i = 0; i < _initSize; i++) {
    Connection *pconn = new Connection();
    pconn->connect(_ip, _user, _password, _dbname, _port);
    pconn->refreshAliveTime(); // 刷新一下开始空闲的起始时间
    _connectionQue.push(pconn);
    _connectionCnt++;
  }

  // 启动一个新的线程，作为连接的生产者
  thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
  produce.detach();

  // 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行对于的连接回收
  thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
  scanner.detach();
}

// 运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask() {
  for (;;) {
    unique_lock<mutex> lock(_queueMutex);
    while (!_connectionQue.empty()) {
      cv.wait(lock); // 队列不空，此处生产线程进入等待状态
    }

    // 连接数量没有到达上限，继续创建新的连接
    if (_connectionCnt < _maxSize) {
      Connection *pconn = new Connection();
      pconn->connect(_ip, _user, _password, _dbname, _port);
      pconn->refreshAliveTime(); // 刷新一下开始空闲的起始时间
      _connectionQue.push(pconn);
      _connectionCnt++;
    }

    // 通知消费者线程，可以消费连接了
    cv.notify_all();
  }
}

// 给外部提供接口，从连接池中获取一个可用的空闲连接
shared_ptr<Connection> ConnectionPool::getConnection() {
  unique_lock<mutex> lock(_queueMutex);
  while (_connectionQue.empty()) {
    // sleep
    if (cv_status::timeout ==
        cv.wait_for(lock, chrono::milliseconds(_connectionTimeout))) {
      if (_connectionQue.empty()) {
        std::cout << __FILE__ << ":" << __LINE__ << " " << __TIMESTAMP__
                  << " : 获取空闲连接超时了...获取连接失败!" << std::endl;
        return nullptr;
      }
    }
  }
  /*
  shared_ptr智能指针析构时，会把connection资源直接delete掉，相当于
  调用connection的析构函数，connection就被close掉了。
  这里需要自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
  */
  shared_ptr<Connection> sp(_connectionQue.front(), [&](Connection *pconn) {
    // 这里是在服务器应用线程中调用的，所以一定要考虑队列的线程安全操作
    unique_lock<mutex> lock(_queueMutex);
    pconn->refreshAliveTime(); // 刷新一下开始空闲的起始时间
    _connectionQue.push(pconn);
  });

  _connectionQue.pop();
  cv.notify_all(); // 消费完连接以后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接

  return sp;
}

// 扫描超过maxIdleTime时间的空闲连接，进行对于的连接回收
void ConnectionPool::scannerConnectionTask() {
  for (;;) {
    // 通过sleep模拟定时效果
    this_thread::sleep_for(chrono::seconds(_maxIdleTime));

    // 扫描整个队列，释放多余的连接
    unique_lock<mutex> lock(_queueMutex);
    while (_connectionCnt > _initSize) {
      Connection *pconn = _connectionQue.front();
      if (pconn->getAliveeTime() >= (_maxIdleTime * 1000)) {
        _connectionQue.pop();
        _connectionCnt--;
        delete pconn; // 调用~Connection()释放连接
      } else {
        break; // 队头的连接没有超过_maxIdleTime，其它连接肯定没有
      }
    }
  }
}