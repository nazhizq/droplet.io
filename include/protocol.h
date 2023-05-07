
#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include <ctime>
#include "json.hpp"
#include "buffer.h"
#include "nio.h"

namespace droplet { 

using json = nlohmann::json;

enum MsgType {
    shutdown = 0x0001,
    reboot = 0x0002,
    
    shutdown_resp = 0x2001,
    reboot_resp = 0x2002,
};

enum ErrCode {
    OK = 0x0,
    MsgFormateErr,
    MsgTypeErr,
};

template<typename T>
void get_json_to_field(json &j, const std::string& key, T& val) {
    if (j.find(key) != j.end()) {
        j.at(key).get_to(val);
    }
};

struct RpcHead {
    unsigned short flag = 0x55AA;
    unsigned int length;
    unsigned short msg_type;
    unsigned short msg_data_fmt = 0;
};

struct RpcData {
    unsigned short json_len;
    std::string json_data = "{}";
    Buffer bin_data;
};

class RpcMessage {
public:
    RpcMessage() {

    }

    virtual ~RpcMessage() {

    }

    virtual int package_size() {
        return RPC_HEAD_LENGTH + RPC_JSON_LENGTH + msg_data.json_data.size() + msg_data.bin_data.size() + RPC_TAIL_LENGTH;
    }

    virtual int rpc_pack(void* buf, int len) {
        if (!buf || !len || msg_data.json_data.size() == 0) return -1;
        msg_data.json_len = msg_data.json_data.size();
        unsigned int bin_data_length = msg_data.bin_data.size();
        unsigned int length = RPC_HEAD_LENGTH + RPC_JSON_LENGTH + msg_data.json_len + bin_data_length + RPC_TAIL_LENGTH;
        if (len < int(length)) {
            return -2;
        }
        unsigned char* p = (unsigned char*)buf;
        *p++ = head.flag & 0xFF;
        *p++ = (head.flag >> 8)  & 0xFF;
        unsigned char* q = p;
        // head.length
        *p++ = length         & 0xFF;
        *p++ = (length >> 8)  & 0xFF;
        *p++ = (length >> 16) & 0xFF;
        *p++ = (length >> 24) & 0xFF;
        // msg_type
        *p++ = head.msg_type & 0xFF;
        *p++ = (head.msg_type >> 8)  & 0xFF;
        // msg_data_fmt
        *p++ = head.msg_data_fmt & 0xFF;
        *p++ = (head.msg_data_fmt >> 8)  & 0xFF;
        // data
        *p++ = msg_data.json_len & 0xFF;
        *p++ = (msg_data.json_len >> 8)  & 0xFF;
        memcpy(p, msg_data.json_data.c_str(), msg_data.json_len);
        p += msg_data.json_len;
        if (bin_data_length > 0) {
            memcpy(p, msg_data.bin_data.data(), bin_data_length);
            p += bin_data_length;
        }
        // end
        *p++ = tail & 0xFF;
        *p++ = (tail >> 8)  & 0xFF;
        return 0;
    }

    virtual int rpc_pack_head(void* buf, int len) {
        if (!buf || !len || msg_data.json_data.size() == 0) return -1;
        msg_data.json_len = msg_data.json_data.size();
        unsigned int bin_data_length = msg_data.bin_data.size();
        unsigned int length = RPC_HEAD_LENGTH + RPC_JSON_LENGTH + msg_data.json_len + bin_data_length + RPC_TAIL_LENGTH;
        if (len < int(length)) {
            return -2;
        }
        unsigned char* p = (unsigned char*)buf;
        *p++ = head.flag & 0xFF;
        *p++ = (head.flag >> 8)  & 0xFF;
        unsigned char* q = p;
        // head.length
        *p++ = length         & 0xFF;
        *p++ = (length >> 8)  & 0xFF;
        *p++ = (length >> 16) & 0xFF;
        *p++ = (length >> 24) & 0xFF;
        // msg_type
        *p++ = head.msg_type & 0xFF;
        *p++ = (head.msg_type >> 8)  & 0xFF;
        // msg_data_fmt
        *p++ = head.msg_data_fmt & 0xFF;
        *p++ = (head.msg_data_fmt >> 8)  & 0xFF;
        return 0;
    }

    virtual int rpc_unpack(const void* buf, int len) {
        if (!buf || !len) return -1;
        if (len < RPC_HEAD_LENGTH) return -2;
        const unsigned char* p = (const unsigned char*)buf;
        head.flag = *p++;
        head.flag |= ((unsigned int)*p++) << 8;

        head.length = *p++;
        head.length |= ((unsigned int)*p++) << 8;
        head.length |= ((unsigned int)*p++) << 16;
        head.length |= ((unsigned int)*p++) << 24;

        // msg_type
        head.msg_type = *p++;
        head.msg_type |= ((unsigned int)*p++) << 8;
        // msg_data_fmt
        head.msg_data_fmt = *p++;
        head.msg_data_fmt |= ((unsigned int)*p++) << 8;

        // Check is buffer enough
        if (len < head.length) {
            return -3;
        }
        // json_len
        msg_data.json_len = *p++;
        msg_data.json_len |= ((unsigned int)*p++) << 8;
        unsigned int bin_data_len = head.length - RPC_HEAD_LENGTH - RPC_JSON_LENGTH - msg_data.json_len - RPC_TAIL_LENGTH;
        msg_data.json_data.resize(msg_data.json_len);
        memcpy((void*)msg_data.json_data.data(), p, msg_data.json_len);
        p += msg_data.json_len;
        if (bin_data_len > 0) {
            msg_data.bin_data.copy((void*)p, bin_data_len);
        }
        return 0;
    }

    virtual int rpc_unpack_head(const void* buf, int len) {
        if (!buf || !len) return -1;
        if (len < RPC_HEAD_LENGTH) return -2;
        const unsigned char* p = (const unsigned char*)buf;
        head.flag = *p++;
        head.flag |= ((unsigned int)*p++) << 8;

        head.length = *p++;
        head.length |= ((unsigned int)*p++) << 8;
        head.length |= ((unsigned int)*p++) << 16;
        head.length |= ((unsigned int)*p++) << 24;

        // msg_type
        head.msg_type = *p++;
        head.msg_type |= ((unsigned int)*p++) << 8;
        // msg_data_fmt
        head.msg_data_fmt = *p++;
        head.msg_data_fmt |= ((unsigned int)*p++) << 8;
        return 0;
    }

    virtual int rpc_unpack_data(const void* buf, int len) {
        // Check is buffer enough
        if (len < head.length - RPC_HEAD_LENGTH) {
            return -3;
        }
        const unsigned char* p = (const unsigned char*)buf;
        // json_len
        msg_data.json_len = *p++;
        msg_data.json_len |= ((unsigned int)*p++) << 8;
        int bin_data_len = head.length - RPC_HEAD_LENGTH - RPC_JSON_LENGTH - msg_data.json_len - RPC_TAIL_LENGTH;
        if (msg_data.json_len > 0) {
            msg_data.json_data.resize(msg_data.json_len);
            memcpy((void*)msg_data.json_data.data(), p, msg_data.json_len);
            p += msg_data.json_len;
        }
        if (bin_data_len > 0) {
            msg_data.bin_data.copy((void*)p, bin_data_len);
        }
        return 0;
    }

    virtual int recv_data(int fd) {
        Buffer req_head_buf(RPC_HEAD_LENGTH);
        char* buf_ptr = (char*)req_head_buf.data();
        int ret_code = Nio::io_read(fd, buf_ptr, RPC_HEAD_LENGTH);
        if (0 != ret_code) {
            return ret_code;
        }
        if (0 != rpc_unpack_head(buf_ptr, RPC_HEAD_LENGTH)) {
            return -2;
        }
        if (head.length - RPC_HEAD_LENGTH <= 0) {
            return -3;
        }
        Buffer req_msg_buf(head.length - RPC_HEAD_LENGTH);
        buf_ptr = (char*)req_msg_buf.data();
        ret_code = Nio::io_read(fd, buf_ptr, head.length - RPC_HEAD_LENGTH);
        if (0 != ret_code) {
            return ret_code;
        }
        if (0 != rpc_unpack_data(buf_ptr, head.length - RPC_HEAD_LENGTH)) {
            return -5;
        }
        return 0;
    }

    virtual int send_data(int fd) {
        Buffer resp_buf(RPC_HEAD_LENGTH + RPC_JSON_LENGTH + msg_data.json_data.size() + msg_data.bin_data.size() + RPC_TAIL_LENGTH);
        if (0 != rpc_pack(resp_buf.data(), (int)resp_buf.size())) {
            return -1;
        }
        int ret_code = Nio::io_write(fd, (char*)resp_buf.data(), resp_buf.size());
        if (0 != ret_code) {
            return ret_code;
        }
        return 0;
    }

    virtual int send_udp_msg(int fd, RpcMessage& req_msg, sockaddr* from_addr, socklen_t addr_len) {
        Buffer req_buf(package_size());
        if (0 != rpc_pack(req_buf.data(), req_buf.size())) {
            return -1;
        }
        char* buf_ptr = (char*)req_buf.data();
        char* p = (char*)req_buf.data();
        int len = req_buf.size();
        int total = 0;
        while(total < len) {
            int nwrite = sendto(fd, p + total, len - total, 0, from_addr, addr_len);
            if (nwrite < 0) {
                int err = errno;
                return err;
            }
            total += nwrite;
        }
        return 0;
    }

    virtual int recv_udp_msg(int fd, RpcMessage& req_msg, sockaddr* from_addr, socklen_t* addr_len) {
        Buffer req_head_buf(RPC_HEAD_LENGTH);
        char* buf_ptr = (char*)req_head_buf.data();
        int ret_code = Nio::io_read_udp(fd, buf_ptr, RPC_HEAD_LENGTH, MSG_PEEK, from_addr, addr_len);
        if (0 != ret_code) {
            return ret_code;
        }
        if (0 != rpc_unpack_head(buf_ptr, RPC_HEAD_LENGTH)) {
            return -2;
        }
        if (head.length - RPC_HEAD_LENGTH <= 0) {
            return -3;
        }
        Buffer req_msg_buf(head.length);
        buf_ptr = (char*)req_msg_buf.data();
        ret_code = Nio::io_read_udp(fd, buf_ptr, head.length, 0, from_addr, addr_len);
        if (0 != ret_code) {
            return ret_code;
        }
        buf_ptr += RPC_HEAD_LENGTH;
        if (0 != rpc_unpack_data(buf_ptr, head.length - RPC_HEAD_LENGTH)) {
            return -5;
        }
        return 0;
    }

public:
    RpcHead head;
    RpcData msg_data;
    unsigned short tail = 0xAA55;
};

class SelfRpcMsg : public RpcMessage {

};

}