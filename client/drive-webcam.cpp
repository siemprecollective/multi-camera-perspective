#include "../lib/arduino-serial-lib.h"
#include "../lib/PracticalSocket.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
    //int fd = serialport_init(argv[1], 9600);
    //serialport_flush(fd);
    char* server = argv[2];
    int port = std::atoi(argv[3]);
    char* id = argv[4];

    TCPSocket sock(server, port);
    sock.send("recv", 5);
    uint64_t len = std::strlen(id);
    sock.send(&len, sizeof(len));
    sock.send(id, std::strlen(id));

    while(true) {
        uint8_t byte;
        sock.recv(&byte, 1);
        std::cout << (int) byte << std::endl;
        //serialport_writebyte(fd, byte);
    }
}
