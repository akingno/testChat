#include <iostream>
#include "AQNThreadPool.h"
#include <arpa/inet.h>
#include "MessageDispatcher.h"


#define PORT 8080
#define NUM_THREADS 5

decltype(auto) initial_socket(int server_fd){
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

  auto server_address = initial_socket(server_fd);

  if (listen(server_fd, 3) < 0) {
    std::cerr << "Listen failed!" << std::endl;
    close(server_fd);
    return -1;
  }

  std::cout << "Server is listening on port " << PORT << "..." << std::endl;

  AQNTHREADPOOL threadPool(NUM_THREADS); // 创建一个包含4个线程的线程池

  std::cout << "ThreadPool ready" << std::endl;

  while (true) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
    if (client_socket < 0) {
      std::cerr << "Client connection failed!" << std::endl;
      continue;
    }
    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET,
              &client_addr.sin_addr,
              client_ip,
              sizeof(client_ip));

    std::cout << "Client connected from: " << client_ip << std::endl;

    // 创建包含客户端信息的任务
    auto info = std::make_shared<InfoFromClient>(0, client_ip, client_socket);

    MessageDispatcher::getInstance().registerClient(client_socket, info);

    threadPool.addTask(info);


  }

  close(server_fd);
  std::cout << "Server closed" << std::endl;
  return 0;
}
