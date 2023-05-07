English | [中文](README-CN.md)

# droplet.io

[![release](https://badgen.net/github/release/nazhizq/droplet.io?icon=github)](https://github.com/nazhizq/droplet.io/releases)
[![stars](https://badgen.net/github/stars/nazhizq/droplet.io?icon=github)](https://github.com/nazhizq/droplet.io/stargazers)
[![forks](https://badgen.net/github/forks/nazhizq/droplet.io?icon=github)](https://github.com/nazhizq/droplet.io/network/members)
[![issues](https://badgen.net/github/issues/nazhizq/droplet.io?icon=github)](https://github.com/nazhizq/droplet.io/issues)
[![PRs](https://badgen.net/github/prs/nazhizq/droplet.io?icon=github)](https://github.com/nazhizq/droplet.io/pulls)
<br>

## ✨ Features
- Small as it is, the sparrow has all the vital organs
- Simple. Header-Only, all code in include directory
- Single-Minded. Only Linux Platform, Only Epoll
- Safe. MultiThread-safe read and write
- High-performance EventLoop, IO multiplexing, Event Callback
- High-Flexibility. Support custom secondary development
- TCP client/server
- Built-in common unpacking mode(LengthField)


## ⌛️ Build

Makefile:
```shell
make
```

### TCP
#### tcp server
**c++ version**: [examples/tcp_server_main.cpp](examples/tcp_server_main.cpp)

```c++
#include "tcp_server.h"
#include "protocol.h"

using namespace ::droplet;
int main() {
    TcpServer server;
    auto onMessage = [](std::shared_ptr<EioChannel>& chan, std::shared_ptr<Buffer>& read_buf) {
        RpcMessage rpc_msg;
        rpc_msg.rpc_unpack(read_buf->data(), read_buf->size());
        rpc_msg.head.msg_type = MsgType::shutdown_resp;
        rpc_msg.msg_data.json_data = "[1,2,3,4]";
        rpc_msg.msg_data.bin_data.resize(1024*1024*20);
        memset(rpc_msg.msg_data.bin_data.data(), 0, rpc_msg.msg_data.bin_data.size());
        auto resp_buff = std::make_shared<Buffer>(rpc_msg.package_size());
        rpc_msg.rpc_pack(resp_buff->data(), resp_buff->size());
        chan->send_buf(resp_buff);
        std::cout << "main onMessage. msg_type: " << rpc_msg.head.msg_type
        << " json_data: " << rpc_msg.msg_data.bin_data.size() << std::endl;
    };
    UnpackSetting setting;
    setting.length_field_coding = CODING_LITTLE_ENDIAN;
    setting.length_field_offset = RPC_HEAD_LENGTH_FIELD_OFFSET;
    setting.length_field_bytes = RPC_HEAD_LENGTH_FIELD_BYTES;
    setting.length_adjustment = 0;
    server.set_unpack_setting(setting);
    server.set_callback_f(onMessage);
    if (0 != server.init(22222, 3)) {
        return -1;
    }
    while(getchar() != '\n');
    return 0;
}
```

#### tcp client
**c++ version**: [examples/tcp_cli_main.cpp](examples/tcp_cli_main.cpp)

```c++
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

using namespace ::droplet;

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
```
