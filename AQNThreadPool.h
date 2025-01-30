#ifndef TESTSERVER2__AQNTHREADPOOL_H_
#define TESTSERVER2__AQNTHREADPOOL_H_

#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <condition_variable>
#include <cstring>
#include <unistd.h>  // for close()
#include <sys/socket.h>
#include <netinet/in.h>

class InfoFromClient {
 public:
   InfoFromClient(long _token, std::string _ip, std::string _name, std::string _message, int client_socket);
   int getClientSocket() const;

  

 private:
  long _token;
  std::string _ip;
  std::string _name;
  std::string _message;
  int _client_socket;
};

class AQNWorker{
 public:
  AQNWorker(int priority,
            std::condition_variable& con_var_task_dispatch,
            std::mutex& mtx,
            std::queue<InfoFromClient>& _infos);


  void work();

 private:
    void handleClient(const InfoFromClient& info);

    int _priority;
    bool _is_working;
    std::thread _worker;
    std::condition_variable& _con_var_task_dispatch;
    std::mutex& _mtx;
    std::queue<InfoFromClient>& _infos;
};


class AQNTHREADPOOL{
 public:
  explicit AQNTHREADPOOL(int numThreads);
  void addTask(const InfoFromClient& info);

 private:
  int _numThreads;
  std::queue<InfoFromClient>  _infos;
  std::vector<AQNWorker>      _workers;
  std::condition_variable _con_var_task_dispatch;
  std::mutex _mtx;

};

#endif//TESTSERVER2__AQNTHREADPOOL_H_