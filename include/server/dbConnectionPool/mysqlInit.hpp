#ifndef MYSQLINIT_H
#define MYSQLINIT_H

#include <string>

using namespace std;

// 数据库连接池配置信息
const string _ip = "127.0.0.1";
const string _user = "root";
const string _password = "000103";
const string _dbname = "user_chat";
const unsigned int _port = 3306;
const unsigned int _initSize = 10;
const unsigned int _maxSize = 1024;
const unsigned int _connectionTimeout = 100;
const unsigned int _maxIdleTime = 60;

#endif