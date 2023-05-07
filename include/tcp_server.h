#pragma once

#include <memory>
#include <iostream>
#include "io_acceptor.h"

namespace sparrow {

class TcpServer {
public:
    TcpServer() : _quit(false) {
        _acceptor_ptr = std::make_shared<IOAcceptor>();
    }

    ~TcpServer() {
        if (!_quit) {
            _quit = true;
        }
    }

    void set_callback_f(
            std::function<void(std::shared_ptr<EioChannel>& chan, std::shared_ptr<Buffer>& read_buf)> onMessage) {
        _acceptor_ptr->onMessage = onMessage;
    }

    void set_unpack_setting(UnpackSetting& setting) {
        _acceptor_ptr->unpack_setting = setting;
    }

    int init(int port, int worker_num) {
        int listen_fd = Nio::listen_socket(port);
        if (listen_fd < 0) {
            return -1;
        }
        _acceptor_ptr->init(listen_fd, 3);
        return 0;
    }

private:
    int _listen_fd;
    bool _quit;
    std::shared_ptr<IOAcceptor> _acceptor_ptr;
};

}