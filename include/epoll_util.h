#pragma once

#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <string.h>
#include "nio.h"

namespace sparrow {

#define MAXLINE 1024

struct EioEvent{
    int fd = -1;
    int events = 0;
    EioEvent(int fd, int events) {
        this->fd = fd;
        this->events = events;
    }
};

struct EpollCtx {
    int listen_fd;
    int epoll_fd;
    struct epoll_event event_arr[EVENTS_INIT_SIZE];
};

class EpollUtil {
public:
    EpollUtil() {}
    ~EpollUtil() {}
    static int create(EpollCtx& ep_ctx) { 
        ep_ctx.epoll_fd = epoll_create(EVENTS_INIT_SIZE);
        EioEvent ev(ep_ctx.listen_fd, 0);
        add_event(ep_ctx, ev, EIO_READ, 0);
        return 0;
    }

    static int create(int& epoll_fd) { 
        epoll_fd = epoll_create(EVENTS_INIT_SIZE);
        return 0;
    }

    static int add_event(EpollCtx& ep_ctx, EioEvent& ev, int events, int flag = EPOLLET) {
        int op = ev.events == 0 ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
        struct epoll_event ee;
        memset(&ee, 0, sizeof(ee));
        ee.data.fd = ev.fd;
        if (ev.events & EIO_READ) {
            ee.events |= EPOLLIN;
        }
        if (ev.events & EIO_WRITE) {
            ee.events |= EPOLLOUT;
        }
        if (events & EIO_READ) {
            ee.events |= EPOLLIN;
        }
        if (events & EIO_WRITE) {
            ee.events |= EPOLLOUT;
        }
        ee.events |= flag;
        epoll_ctl(ep_ctx.epoll_fd, op, ev.fd, &ee);
        return 0;
    }

    static int modify_event(EpollCtx& ep_ctx, int fd, int events, int flag = EPOLLET) {
        struct epoll_event ee;
        memset(&ee, 0, sizeof(ee));
        ee.data.fd = fd;
        if (events & EIO_READ) {
            ee.events |= EPOLLIN;
        }
        if (events & EIO_WRITE) {
            ee.events |= EPOLLOUT;
        }
        ee.events |= flag;
        epoll_ctl(ep_ctx.epoll_fd, EPOLL_CTL_MOD, fd, &ee);
        return 0;
    }

    static int del_event(EpollCtx& ep_ctx, EioEvent& ev, int events, int flag = EPOLLET) {
        struct epoll_event ee;
        memset(&ee, 0, sizeof(ee));
        ee.data.fd = ev.fd;
        if (ev.events & EIO_READ) {
            ee.events |= EPOLLIN;
        }
        if (ev.events & EIO_WRITE) {
            ee.events |= EPOLLOUT;
        }
        if (events & EIO_READ) {
            ee.events &= ~EPOLLIN;
        }
        if (events & EIO_WRITE) {
            ee.events &= ~EPOLLOUT;
        }
        if (ee.events == 0) {
            epoll_ctl(ep_ctx.epoll_fd, EPOLL_CTL_DEL, ev.fd, &ee);
        } else {
            ee.events |= flag;
            epoll_ctl(ep_ctx.epoll_fd, EPOLL_CTL_MOD, ev.fd, &ee);
        }
        return 0;
    }

    static int wait_events(EpollCtx& ep_ctx, std::vector<EioEvent>& eio_events) {
        // std::unique_lock<std::mutex> lock(_mx); 
        int nfds = epoll_wait(ep_ctx.epoll_fd , ep_ctx.event_arr, EVENTS_INIT_SIZE, 500);
        for (int i = 0; i < nfds; ++i) {
            int sockfd = ep_ctx.event_arr[i].data.fd;
            if(ep_ctx.event_arr[i].events&EPOLLIN) {
                EioEvent ev(sockfd, EIO_READ);
                eio_events.emplace_back(ev);
            } else if(ep_ctx.event_arr[i].events&EPOLLOUT) {
                EioEvent ev(sockfd, EIO_WRITE);
                eio_events.emplace_back(ev);
            }
        }
        return 0;
    }

};

}