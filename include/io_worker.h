#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "epoll_util.h"
#include "concurrent_queue.h"
#include "channel.h"
#include "nio.h"

namespace droplet {

class IoWorker {
public:
    IoWorker() : _quit(false) {

    }

    ~IoWorker() {
        if (!_quit) {
            _quit = true;
        }
        if (_thread_ptr != nullptr) {
            _thread_ptr->join();
        }
    }

    int init(int index) {
        _index = index;
        _io_ctx = std::make_shared<IoCtx>();;
        EpollUtil::create(_io_ctx->epoll_ctx->epoll_fd);
        _thread_ptr = std::make_shared<std::thread>(&IoWorker::run, this);
        return 0;
    }

    std::shared_ptr<EioChannel> get_chan(int fd) {
        auto chan = _io_ctx->get_chan(fd);
        chan->epoll_ctx = _io_ctx->epoll_ctx;
        return chan;
    }

    void watch_conn(EioEvent& ev) {
        setnonblocking(ev.fd);
        EpollUtil::add_event(*_io_ctx->epoll_ctx, ev, EIO_READ);
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
                    std::cout << "thread_i: " << " fd: " << ev.fd << " err." << std::endl;
                    continue;
                }
                std::shared_ptr<EioChannel> chan = get_chan(ev.fd);
                if (chan == nullptr || chan == nullptr) {
                    std::cout << "thread_i: " << _index << " thread_i: " << _index << " fd: " << ev.fd << " not in channel_arr." << std::endl;
                    continue;
                }
                chan->events = ev.events;
                // std::cout << "thread_i: " << _index << " thread_i: " << _index << " handler fd: " << chan->fd << " events: " << chan->events 
                //     << " write_queue_size: " << chan->write_queue_size() << std::endl;
                if (chan->events & EIO_READ) {
                    auto read_buf = std::make_shared<Buffer>();
                    int ret_code = chan->recv_buf(read_buf);
                    if (0 != ret_code) {
                        if (IoErrCode::IoDisconnect == ret_code) {
                            // close(chan->fd);
                        } else {
                            std::cout << "thread_i: " << _index << " recv_buf connfd: " << chan->fd << " ret_code: " << ret_code << std::endl;
                        }
                        continue;
                    }
                    chan->onMessage(chan, read_buf);
                }
                if (chan->events & EIO_WRITE) {
                    while (chan->write_queue_size() > 0) {
                        auto msg_buf = chan->pop_write_msg();
                        int ret_code = Nio::io_write(chan->fd, (char*)msg_buf->data(), msg_buf->size());
                        if (0 != ret_code) {
                            if (IoErrCode::IoDisconnect == ret_code) {
                                // close(chan->fd);
                            } else {
                                std::cout << "thread_i: " << _index << " write connfd: " << chan->fd << " ret_code: " << ret_code << std::endl;
                            }
                            continue;
                        }
                    } 
                    EpollUtil::modify_event(*_io_ctx->epoll_ctx, chan->fd, EIO_READ);
                }
            }
        }
    }

private:
    bool _quit;
    int _index;
    std::shared_ptr<std::thread> _thread_ptr;
    std::shared_ptr<IoCtx> _io_ctx;
};

}