#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include "io_worker.h"
#include "epoll_util.h"
#include "concurrent_queue.h"
#include "channel.h"

namespace droplet {

class IOAcceptor {
public:
    IOAcceptor() : _quit(false) {

    }

    ~IOAcceptor() {
        if (!_quit) {
            _quit = true;
        }
        if (_thread_ptr != nullptr) {
            _thread_ptr->join();
        }
    }

    int init(int listen_fd, int worker_num) {
        _io_ctx = std::make_shared<IoCtx>();
        _io_ctx->epoll_ctx->listen_fd = listen_fd;
        EpollUtil::create(*_io_ctx->epoll_ctx);
        _thread_ptr = std::make_shared<std::thread>(&IOAcceptor::run, this);
        for (int i=0; i<worker_num; i++) {
            auto worker_ptr = std::make_shared<IoWorker>();
            worker_ptr->init(i);
            _worker_list.emplace_back(worker_ptr);
        }
        return 0;
    }

    void run() {
        while (!_quit) {
            std::vector<EioEvent> poll_events;
            EpollUtil::wait_events(*_io_ctx->epoll_ctx, poll_events);
            if (poll_events.empty()) {
                // std::cout << "poll_events continue. size: " << poll_events.size() << std::endl;
                continue;
            }
            struct sockaddr_in client_addr;
            socklen_t cli_len = sizeof(client_addr);
            for (auto& ev : poll_events) {
                if (ev.fd <= 0) {
                    continue;
                }
                // std::cout << "acceptor poll_event ev.fd: " << ev.fd << " listen_fd: " << _io_ctx->listen_fd << std::endl;
                if (ev.fd == _io_ctx->epoll_ctx->listen_fd) {
                    int connfd = accept(ev.fd, (sockaddr *)&client_addr, &cli_len);
                    if (connfd < 0) {
                        std::cout << "accept listen_fd: " << ev.fd << " exception. errno: " << errno << std::endl;
                        continue;
                    }
                    // std::cout << "accept new connfd: " << connfd << " listen_fd: " << ev.fd << std::endl;
                    EioEvent ev(connfd, 0);
                    dispatcher(ev);
                } 
                
            }
        }
    }

    void dispatcher(EioEvent& ev) {
        size_t index = ((size_t)ev.fd) % _worker_list.size();
        auto chan_ptr = _worker_list[index]->get_chan(ev.fd);
        chan_ptr->unpack_setting = unpack_setting;
        chan_ptr->onMessage = onMessage;
        _worker_list[index]->watch_conn(ev);
    }

    private:
        int _listen_fd;
        bool _quit;
        std::shared_ptr<std::thread> _thread_ptr;
        std::shared_ptr<IoCtx> _io_ctx;
        std::vector<std::shared_ptr<IoWorker>> _worker_list;

    public:
        UnpackSetting unpack_setting;
        std::function<void(std::shared_ptr<EioChannel>& chan, std::shared_ptr<Buffer>& read_buf)> onMessage;
    };

}