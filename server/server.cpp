#include "../lib/PracticalSocket.h"

#include <iostream>
#include <cstdlib>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::unordered_map<std::string, TCPSocket*> recv_sockets;
std::vector<std::thread> send_threads;

void handle_send(TCPSocket* client) {
  uint64_t len;
  char* id;
  std::unordered_map<std::string, TCPSocket*>::iterator dest_it;
  if (client->recv(&len, sizeof(len)) <= 0) goto cleanup_send;
  id = new char[len];
  if (client->recv(id, len) <= 0) goto cleanup_recv_id;

  while (true) {
    uint8_t byte;
    if (client->recv(&byte, 1) <= 0) goto cleanup_recv_id;
    dest_it = recv_sockets.find(std::string(id));
    if (dest_it == recv_sockets.end()) continue;
    try {
      dest_it->second->send(&byte, 1);
    } catch (SocketException& e) {
      // TODO this is not thread safe
      std::cout << e.what() << std::endl;
      delete dest_it->second;
      recv_sockets.erase(dest_it);
    }
  }

cleanup_recv_id:
  delete[] id;
cleanup_send:
  std::cout << "cleaning up sender" << std::endl;
  delete client;
  return;
}

void handle_recv(TCPSocket* client) {
      uint64_t len;
      char* id;
      if (client->recv(&len, sizeof(len)) <= 0) goto end;
      id = new char[len];
      if (client->recv(id, len) <= 0) goto cleanup;
      recv_sockets[std::string(id)] = client;
cleanup:
      delete[] id;
end:
      return;
}

int main(int argc, char** argv) {
  signal(SIGPIPE, SIG_IGN);

  int port = std::atoi(argv[1]);
  TCPServerSocket sock(port);

  while (true) {
    TCPSocket* client = sock.accept();
    char type_buf[5];
    client->recv(&type_buf, 5);
    std::string type(type_buf);
    if (type == "recv") {
      handle_recv(client);
    } else if (type == "send") {
      std::thread thread(handle_send, client);
      send_threads.push_back(std::move(thread));
    }
  }
}
