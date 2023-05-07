#pragma once

#include <memory>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
// #include <sys/types.h>
#include <fcntl.h>

namespace sparrow {

#define EVENTS_INIT_SIZE 64
#define MAX_LINK_NUM 1024

#define EIO_READ  0x0001
#define EIO_WRITE 0x0004
#define IO_RDWR  (EIO_READ|EIO_WRITE)

#define setblocking(s)     fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)
#define setnonblocking(s)  fcntl(s, F_SETFL, fcntl(s, F_GETFL) |  O_NONBLOCK)

#define RPC_HEAD_LENGTH                10
#define RPC_HEAD_LENGTH_FIELD_OFFSET   2
#define RPC_HEAD_LENGTH_FIELD_BYTES    4
#define RPC_JSON_LENGTH                2
#define RPC_TAIL_LENGTH                2

#define CODING_LITTLE_ENDIAN 1
#define CODING_BIG_ENDIAN    2

struct UnpackSetting {
    unsigned short length_field_coding = CODING_LITTLE_ENDIAN;
    unsigned int body_offset = RPC_HEAD_LENGTH;
    unsigned int length_field_offset = RPC_HEAD_LENGTH_FIELD_OFFSET;
    unsigned int length_field_bytes = RPC_HEAD_LENGTH_FIELD_BYTES;
    int  length_adjustment = 0;
};

enum IoErrCode {
    IoDisconnect = 0x8001,
};

class Nio {
public:
    static int io_errno() {
        return errno;
    }
    static int io_read(int fd, char* buf, int len) {
        char* p = (char*)buf;
        int total = 0;
        while(total < len) {
            int nread = read(fd, p + total, len - total);
            if (nread == 0) {
                return IoErrCode::IoDisconnect;
            } else if (nread < 0) {
                int err = io_errno();
                if (err == EAGAIN) {
                    continue;
                }  else {
                    std::cout << "io_read err:" << err << std::endl;
                }
                return err;
            }
            total += nread;
            continue;
        }
        return 0;
    }

    static int io_write(int fd, char* buf, int len) {
        if (len <= 0) {
            return 0;
        }
        char* p = (char*)buf;
        int total = 0;
        while(total < len) {
            int nwrite = write(fd, p + total, len - total);
            if (nwrite == 0) {
                return IoErrCode::IoDisconnect;
            } else if (nwrite < 0) {
                int err = io_errno();
                if (err == EAGAIN) {
                    continue;
                } else {
                    std::cout << "io_write err:" << err << std::endl;
                }
                return err;
            }
            total += nwrite;
            continue;
        }
        return 0;
    }

    static int io_read_udp(int fd, char* buf, int len, int flag, sockaddr* from_addr, socklen_t* addr_len) {
        char* p = (char*)buf;
        int total = 0;
        while(total < len) {
            int nread = recvfrom(fd, p + total, len - total, flag, from_addr, addr_len);
            char* buf_ptr = buf;
            if (nread < 0) {
                int err = io_errno(); // EAGAIN;
                if (err == EAGAIN || err == EINTR) {
                    continue;
                } else {
                    std::cout << "io_read_udp err:" << err << std::endl;
                }
                return err;
            }
            total += nread;
            continue;
        }
        return 0;
    }

    static int listen_socket(int port) {
        int listen_fd = -1;
        if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
            std::cout << "create socket error: " << strerror(io_errno()) << " errno: " << io_errno() << std::endl;
            return -1;
        }
        sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(struct sockaddr_in));
        server_addr.sin_family = AF_INET;    
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
        server_addr.sin_port = htons(port); 
        int on = 1;
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
            std::cout << "setsockopt error: " << strerror(io_errno()) << " errno: " << io_errno() << std::endl;
            return -1;
        }
        if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cout << "bind socket error: " << strerror(io_errno()) << " errno: " << io_errno() << std::endl;
            return -1;
        }
        if(listen(listen_fd, 20) < 0) {
            printf("listen Error: %s (errno: %d)\n", strerror(io_errno()), io_errno());
            exit(0);
        }
        std::cout << "listen_socket port: " << port << std::endl;
        return listen_fd;
    }

    static int connect_socket(const std::string& ip_str, int port, int recv_timeout) {
        int conn_fd = -1;
        conn_fd = socket(PF_INET, SOCK_STREAM, 0);
        if (conn_fd < 0) {
            std::cout << "Error: open socket err_code: " << io_errno() << " conn_fd: " << conn_fd << std::endl;
            return -1;
        }
        // connect
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip_str.c_str(), &server_addr.sin_addr.s_addr);
        if (connect(conn_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cout << "Error: connect to " << ip_str << " port: " << port << " err_code: " << io_errno() << std::endl;
            close(conn_fd);
            return -1;
        }
        // 设置接收超时时间
        struct timeval tv = {recv_timeout/1000, (recv_timeout%1000)*1000};
        if (setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            std::cout << "Error: setsockopt " << ip_str << " port: " << port << " err_code: " << io_errno() << std::endl;
            close(conn_fd);
            return -1;
        }
        return conn_fd;
    }
};

}