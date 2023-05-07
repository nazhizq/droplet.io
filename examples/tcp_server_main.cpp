#include "tcp_server.h"
#include "protocol.h"

using namespace ::sparrow;
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