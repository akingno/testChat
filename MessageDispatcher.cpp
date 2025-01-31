//
// Created by jacob on 2025/2/1.
//
#include "AQNThreadPool.h"
#include "MessageDispatcher.h"


InfoFromClient::InfoFromClient(long token, std::string ip, int client_socket)
    : _token(token), _ip(std::move(ip)), _client_socket(client_socket),
      _send_mutex_ptr(std::make_shared<std::mutex>()),
      _send_cond_ptr(std::make_shared<std::condition_variable>()) {}

// InfoFromClient::InfoFromClient(const InfoFromClient& other)
//     : _token(other._token), _ip(other._ip), _client_socket(other._client_socket),
//       _send_mutex_ptr(std::make_shared<std::mutex>()),
//       _send_cond_ptr(std::make_shared<std::condition_variable>()) {

//     std::lock_guard<std::mutex> lock(*other._send_mutex_ptr);
//     _send_queue = other._send_queue;
// }

int InfoFromClient::getClientSocket() const { return _client_socket; }

void InfoFromClient::enqueueMessage(const std::string& message) {
  std::cout<<"[DEBUG] info: enqueue begins, ";
  std::lock_guard<std::mutex> lock(*_send_mutex_ptr);
  _send_queue.push(message);
  _send_cond_ptr->notify_one(); // 通知工作线程有新消息
  std::cout<<"enqueue end"<<std::endl;
}
std::string InfoFromClient::dequeueMessage() {
  std::cout<<"[DEBUG] info: dequeue begins, ";
  std::unique_lock<std::mutex> lock(*_send_mutex_ptr);
  if (_send_queue.empty()) {
    return ""; // 返回空消息或抛出异常
  }
  std::string message = _send_queue.front();
  _send_queue.pop();
  return message;
}


//------------------------------------Worker-------------------------------------------------------------------------------

AQNWorker::AQNWorker(int priority,
                     std::shared_ptr<std::condition_variable> con_var,
                     std::shared_ptr<std::mutex> mtx,
                     std::shared_ptr<std::queue<std::shared_ptr<InfoFromClient>>> infos)
    : _priority(priority), _is_working(false), _con_var_task_dispatch(con_var), _mtx(mtx), _infos(infos) {
  _worker = std::thread(&AQNWorker::work, this);
}

AQNWorker::~AQNWorker() {
  if (_worker.joinable()) {
    _worker.join();
  }
}


// 处理客户端连接
void AQNWorker::handleClient(InfoFromClient& info) {
  std::cout << "[Debug] Handling" << std::endl;
  int client_socket = info.getClientSocket();
  // 设置为非阻塞模式
  fcntl(client_socket, F_SETFL, O_NONBLOCK);

  while (true) {
    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(client_socket, &read_fds);
    //std::cout << "[Debug] Handling, handleClient:while:set socket" << std::endl;

    bool has_data = false;
    {
      //std::lock_guard<std::mutex> lock(*info.getSendMutex());
      has_data = !info.isMessageQueueEmpty();
      //std::cout << "[Debug] Message queue size: " << info.getSendQueue().size() << std::endl;
    }
    if (has_data) FD_SET(client_socket, &write_fds);

    //std::cout << "[Debug] Handling, handleClient:while:data existed" << std::endl;

    struct timeval timeout = {1, 0}; // 1秒超时
    int activity = select(client_socket + 1, &read_fds, &write_fds, nullptr, &timeout);

    if (activity < 0) {
      perror("select error");
      break;
    }
    //std::cout << "[Debug] Handling, handleClient:while:handling read" << std::endl;

    // 处理读取
    if (FD_ISSET(client_socket, &read_fds)) {
      char buffer[1024];
      ssize_t valread = recv(client_socket, buffer, sizeof(buffer), 0);
      if (valread > 0) {
        buffer[valread] = '\0';
        std::string message(buffer);

        MessageDispatcher::getInstance().addMessage(message);

        std::cout << "Message from client: " << message << std::endl;

        //send(client_socket, "test2", 5, 0); // 示例回复

      } else if (valread == 0 || (valread < 0 && errno != EAGAIN)) {
        break; // 断开或错误
      }
    }
    //std::cout << "[Debug] Handling, handleClient:while:handling send" << std::endl;

    // 处理发送
    if (FD_ISSET(client_socket, &write_fds)) {
      std::cout << "[Debug] Handling, handleClient:while:handling send:if" << std::endl;
      //td::lock_guard<std::mutex> lock(*info.getSendMutex());
      //std::cout << "[Debug] Entering send loop, queue size: " << info.getSendQueue().size() << std::endl;
      while (!info.isMessageQueueEmpty()) {
        std::string msg = info.dequeueMessage();
        if(msg == ""){
          std::cerr << "Msg is null"<< std::endl;
          break;
        }
        std::cout<<"msg to send: "<<msg<<std::endl;
        ssize_t sent = send(client_socket, msg.c_str(), msg.size(), MSG_DONTWAIT);
        if (sent < 0) {
          std::cerr << "send failed: " << strerror(errno) << std::endl;
          if (errno == EAGAIN) break; // 稍后重试
          else { close(client_socket); return; } // 错误
        } else if (sent < msg.size()) {
          std::cout << "[Debug] Partial send: " << sent << " bytes" << std::endl;
          msg = msg.substr(sent); // 部分发送
          break;
        } else {
          std::cout << "[Debug] Message sent: " << msg << std::endl;
          //info.getSendQueue().pop(); // 发送完成
        }
      }
    }

  }
  close(client_socket);
}

//从infos池子中拿socket并处理
void AQNWorker::work() {
  while (true) {
    std::cout<<"worker " <<_priority << " is working" <<std::endl;

    std::unique_lock<std::mutex> lock(*_mtx);
    _con_var_task_dispatch->wait(lock, [&] { return !_infos->empty(); });

    auto info = _infos->front();
    _infos->pop();
    lock.unlock();

    _is_working = true;
    handleClient(*info);
    _is_working = false;

    MessageDispatcher::getInstance().unregisterClient(info->getClientSocket());

    std::cout<<"[DEBUG] Work end, by worker " <<_priority <<std::endl;

  }
}


//------------------------------------Thread Pool-------------------------------------------------------------------------------

AQNTHREADPOOL::AQNTHREADPOOL(int numThreads) : _numThreads(numThreads),
                                               _con_var_task_dispatch(std::make_shared<std::condition_variable>()),
                                               _mtx(std::make_shared<std::mutex>()),
                                               _infos(std::make_shared<std::queue<std::shared_ptr<InfoFromClient>>>())
{

  _workers.reserve(numThreads);

  for (size_t i = 0; i < numThreads; i++) {
    _workers.emplace_back(std::make_unique<AQNWorker>(i, _con_var_task_dispatch, _mtx, _infos));
    std::cout << "[DEBUG] Thread Created, number: " << i << std::endl;
  }

  std::cout << "[DEBUG] AQNTHREADPOOL constructor end" << std::endl;
}

void AQNTHREADPOOL::addTask(std::shared_ptr<InfoFromClient> info) {
  std::lock_guard<std::mutex> lock(*_mtx);
  _infos->push(info);
  _con_var_task_dispatch->notify_one();
}
