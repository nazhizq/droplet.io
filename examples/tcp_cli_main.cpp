#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include "protocol.h"
#include "nio.h"

using namespace ::sparrow;

int run_get_device_list(int index) {
    int conn_fd = Nio::connect_socket("127.0.0.1", 22222, 3000);
    if (conn_fd <= 0) {
        std::cout << "connect_socket Error. conn_fd: " << conn_fd << std::endl;
        return conn_fd;
    }
    for (size_t i=0; i<100; i++) {
        auto start_t = std::chrono::system_clock::now();
        RpcMessage req_msg;
        RpcMessage resp_data;
        req_msg.head.msg_type = MsgType::shutdown;
        int ret_code = req_msg.send_data(conn_fd);
        if (0 != ret_code) {
            std::cout << "send_tcp_msg ret_code: " << ret_code << std::endl;
            close(conn_fd);
            return 101;
        }
        ret_code = resp_data.recv_data(conn_fd);
        if (0 != ret_code) {
            std::cout << "recv_msg ret_code: " << ret_code << std::endl;
            close(conn_fd);
            return 102;
        }
        auto end_t = std::chrono::system_clock::now();
        int diff_t = std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count();
        std::cout << std::to_string(i) + "," + std::to_string(resp_data.head.length) + "," + std::to_string(diff_t) << std::endl;
    }
    close(conn_fd);
    return 0;
}

int main() {
    int sum = 5;
    std::vector<std::thread> thread_vec;
    for (size_t i=0; i<sum; i++) {
        thread_vec.emplace_back(std::thread(run_get_device_list, i));
    }
    for (size_t i=0; i<sum; i++) {
        thread_vec[i].join();
    }
    return 0;
}