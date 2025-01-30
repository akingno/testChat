#include "AQNThreadPool.h"

InfoFromClient::InfoFromClient(long token, std::string ip, std::string name, std::string message, int client_socket)
      : _token(token), _ip(std::move(ip)), _name(std::move(name)), _message(std::move(message)), _client_socket(client_socket) {}

int InfoFromClient::getClientSocket() const { return _client_socket; }



// 处理客户端连接
void AQNWorker::handleClient(const InfoFromClient& info) {
  std::cout << "Handling" << std::endl;
  int client_socket = info.getClientSocket();
  char buffer[1024] = {0};

  while (true) {
    int valread = read(client_socket, buffer, 1024);
    if (valread > 0) {
      std::cout << "Message from client: " << buffer << std::endl;
      const char* message = "test2";
      send(client_socket, message, strlen(message), 0);
    } else {
      std::cerr << "Client disconnected" << std::endl;
      close(client_socket);
      break;
    }
  }
}


//从infos池子中拿socket并处理
void AQNWorker::work() {
  while (true) {
    //std::cout<<"worker " <<_priority << " is working" <<std::endl;

    std::unique_lock<std::mutex> lock(_mtx);
    _con_var_task_dispatch.wait(lock, [&] { return !_infos.empty(); });

    InfoFromClient info = _infos.front();
    _infos.pop();
    lock.unlock();

    _is_working = true;
    handleClient(info);
    _is_working = false;
  }
}

AQNWorker::AQNWorker(int priority, std::condition_variable& con_var_task_dispatch, std::mutex& mtx, std::queue<InfoFromClient>& infos)
    : _priority(priority), _is_working(false), _con_var_task_dispatch(con_var_task_dispatch), _mtx(mtx), _infos(infos) {
  _worker = std::thread(&AQNWorker::work, this);
}


AQNTHREADPOOL::AQNTHREADPOOL(int numThreads) : _numThreads(numThreads){


  for(size_t i = 0; i < numThreads; i++) {
    _workers.emplace_back(i, _con_var_task_dispatch, _mtx, _infos);
  //std::cout << "[DEBUG] Thread Created, number: " << i << std::endl;
  }
  std::cout << "[DEBUG] AQNTHREADPOOL constructor end" << std::endl;
}

void AQNTHREADPOOL::addTask(const InfoFromClient& info) {
  std::lock_guard<std::mutex> lock(_mtx);
  _infos.push(info);
  _con_var_task_dispatch.notify_one();
}
