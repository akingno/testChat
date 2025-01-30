#include <iostream>
#include "AQNThreadPool.h"


#define PORT 8080
#define NUM_THREADS 5

decltype(auto) initial_socket(int server_fd){
    // 创建 socket
    

    // 设置服务器地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定 socket 到指定的地址和端口
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Binding failed!" << std::endl;
        close(server_fd);
    }
    return address;

}

int main() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    std::cerr << "Socket creation failed!" << std::endl;
    return -1;
  }

  auto address = initial_socket(server_fd);

  if (listen(server_fd, 3) < 0) {
    std::cerr << "Listen failed!" << std::endl;
    close(server_fd);
    return -1;
  }

  std::cout << "Server is listening on port " << PORT << "..." << std::endl;

  AQNTHREADPOOL threadPool(NUM_THREADS); // 创建一个包含4个线程的线程池

  std::cout << "ThreadPool ready" << std::endl;

  while (true) {
    int addrlen = sizeof(address);
    int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_socket < 0) {
      std::cerr << "Client connection failed!" << std::endl;
      continue;
    }

    std::cout << "Client connected!" << std::endl;

    // 创建包含客户端信息的任务
    InfoFromClient info(0, "client_ip", "client_name", "client_message", client_socket);
    threadPool.addTask(info);
  }

  close(server_fd);
  std::cout << "Server closed" << std::endl;
  return 0;
}
