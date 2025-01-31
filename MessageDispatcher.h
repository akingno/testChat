//
// Created by jacob on 2025/2/1.
//

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
#include <sys/select.h>
#include <fcntl.h>

class InfoFromClient {
public:
  InfoFromClient(long _token, std::string _ip, int client_socket);
  InfoFromClient(const InfoFromClient&) = delete;
  InfoFromClient& operator=(const InfoFromClient&) = delete;

  int getClientSocket() const;
  std::shared_ptr<std::mutex>& getSendMutex(){
    return _send_mutex_ptr;
  };

  std::queue<std::string>& getSendQueue(){
    return _send_queue;
  }
  bool isMessageQueueEmpty(){
    std::lock_guard<std::mutex> lock(*_send_mutex_ptr);
    return _send_queue.empty();
  }

  void enqueueMessage(const std::string& message);
  std::string dequeueMessage();



private:
  long _token;
  std::string _ip;

  int _client_socket;
  std::queue<std::string> _send_queue;
  std::shared_ptr<std::mutex> _send_mutex_ptr;
  std::shared_ptr<std::condition_variable> _send_cond_ptr;    //用于通知工作线程的条件变量
};

class AQNWorker{
public:
  AQNWorker(int priority,
            std::shared_ptr<std::condition_variable> con_var,
            std::shared_ptr<std::mutex> mtx,
            std::shared_ptr<std::queue<std::shared_ptr<InfoFromClient>>> infos);


  void work();

  ~AQNWorker();

private:
  void handleClient(InfoFromClient& info);

  int _priority;
  bool _is_working;
  std::thread _worker;
  std::shared_ptr<std::condition_variable> _con_var_task_dispatch;
  std::shared_ptr<std::mutex> _mtx;
  std::shared_ptr<std::queue<std::shared_ptr<InfoFromClient>>> _infos;
};


class AQNTHREADPOOL{
public:
  explicit AQNTHREADPOOL(int numThreads);
  void addTask(std::shared_ptr<InfoFromClient> info);

private:
  int _numThreads;
  std::vector<std::unique_ptr<AQNWorker>> _workers;
  std::shared_ptr<std::condition_variable> _con_var_task_dispatch;
  std::shared_ptr<std::mutex> _mtx;
  std::shared_ptr<std::queue<std::shared_ptr<InfoFromClient>>> _infos;

};

#endif//TESTSERVER2__AQNTHREADPOOL_H_