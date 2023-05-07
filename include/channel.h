#pragma once

#include <mutex>
#include <functional>
#include "concurrent_queue.h"
#include "nio.h"
#include "buffer.h"
#include "epoll_util.h"

namespace droplet {

class EioChannel {
public:
    int fd;
    int events;
    UnpackSetting unpack_setting;
    std::shared_ptr<EpollCtx> epoll_ctx;
    std::function<void(std::shared_ptr<EioChannel>& chan, std::shared_ptr<Buffer>& read_buf)> onMessage;
private:
    std::mutex _mx;
    ConcurrentQueue<std::shared_ptr<Buffer>> write_q;

public:
    EioChannel () : fd(0), events(0) {

    }
    void send_buf(const std::shared_ptr<Buffer>& resp_buf) {
        char* p = (char*)resp_buf->data();
        size_t total = 0;
        size_t len = resp_buf->size();
        if (len <= 0) {
            return;
        }
        int nwrite = 0;
        while(total < len) {
            nwrite = write(fd, p + total, len - total);
            if (nwrite == 0) {
                // close(fd);
                return;
            } else if (nwrite < 0) {
                break;
            }
            total += nwrite;
            continue;
        }
        if (nwrite < 0) {
            if (Nio::io_errno() == EAGAIN) { // writting until cannot write
                p += total;
                auto remain_buf = std::make_shared<Buffer>();
                remain_buf->copy(p, len-total);
                enqueue_write_msg(remain_buf);
                EioEvent ev(fd, events);
                EpollUtil::add_event(*epoll_ctx, ev, EIO_WRITE);
            } else {
                std::cout << "send_buf write err:" << Nio::io_errno() << std::endl;
            }
        }
    }
    int recv_buf(std::shared_ptr<Buffer>& read_buf) {
        size_t head_len = unpack_setting.length_field_offset + unpack_setting.length_field_bytes;
        Buffer head_buf(head_len);
        char* buf_ptr = (char*)head_buf.data();
        int ret_code = Nio::io_read(fd, buf_ptr, head_len);
        if (0 != ret_code) {
            return ret_code;
        }
        unsigned int length = 0;
        const unsigned char* p = (const unsigned char*)buf_ptr;
        p += unpack_setting.length_field_offset;
        for (size_t i = 0; i < unpack_setting.length_field_bytes; i++) {
            if (unpack_setting.length_field_coding == CODING_BIG_ENDIAN) {
                length = (length << 8) | (unsigned int)*p++;
            } else {
                length |= ((unsigned int)*p++) << (i * 8);
            }
        }
        length += unpack_setting.length_adjustment;
        if (length - head_len <= 0) {
            return -1;
        }
        read_buf->resize(length);
        buf_ptr = (char*)read_buf->data();
        memcpy(buf_ptr, head_buf.data(), head_len);
        buf_ptr += head_len;
        ret_code = Nio::io_read(fd, buf_ptr, length - head_len);
        if (0 != ret_code) {
            return ret_code;
        }
        return 0;
    }
    size_t write_queue_size() {
        std::lock_guard<std::mutex> _lg(_mx);
        return write_q.size();
    }
    int enqueue_write_msg(const std::shared_ptr<Buffer>& msg_data) {
        std::lock_guard<std::mutex> _lg(_mx);
        write_q.enqueue(msg_data);
        return 0;
    }
    std::shared_ptr<Buffer> pop_write_msg() {
        std::lock_guard<std::mutex> _lg(_mx);
        return write_q.pop();
    }

};

struct IoCtx {
    std::mutex _mx;
    std::shared_ptr<EpollCtx> epoll_ctx;
    std::vector<std::shared_ptr<EioChannel>> channel_arr;
    
    IoCtx() {
        epoll_ctx = std::make_shared<EpollCtx>();
    }
    std::shared_ptr<EioChannel>& get_chan(int fd) {
        std::lock_guard<std::mutex> _lg(_mx);
        if (fd >= channel_arr.size()) {
            channel_arr.resize((fd+1)*2);
        }
        if (channel_arr[fd].get() == nullptr) {
            channel_arr[fd] = std::make_shared<EioChannel>();
            channel_arr[fd]->fd = fd;
        } 
        return channel_arr[fd];
    }
};

}