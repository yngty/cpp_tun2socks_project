#ifndef LWIP_STACK_H
#define LWIP_STACK_H

#include <string>
#include <vector>
#include <memory>
#include "TunDevice.h"
extern "C" {
#include "lwip/init.h"
#include "lwip/netif.h"
}
class Connection;

class LwipStack {
public:
    LwipStack();
    ~LwipStack();
    void init(const std::string& tun_name, const std::string& socks5_addr, uint16_t socks5_port);
    void start();
    void stop();
    void input(const uint8_t* data, size_t len);
    void onNewTcpConnection(const std::string& remote_ip, uint16_t remote_port);


    bool isTunDeviceOpen() const {
        return tunDevice_ != nullptr && tunDevice_->fd() >= 0;
    }

   ssize_t write(const uint8_t* data, size_t len) {
        if (!tunDevice_) return -1;
        return tunDevice_->write(data, len);
    }
    ssize_t read(void* buffer, size_t size) {
        if (!tunDevice_) return -1;
        return tunDevice_->read(buffer, size);
    }


    void poll() {
        // 这里可以添加轮询逻辑，处理 lwIP 事件
        // 例如调用 netif_poll() 或其他相关函数
    }
    int getTunFd() const { return tunDevice_ ? tunDevice_->fd() : -1; }

private:
    std::unique_ptr<TunDevice> tunDevice_;
    struct netif netif_; // lwIP 网络接口
    std::vector<std::shared_ptr<Connection>> connections_; // 管理所有 TCP  连接
    std::string socks5_host_;
    int socks5_port_;
};
#endif // LWIP_STACK_H
