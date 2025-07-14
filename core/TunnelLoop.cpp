#include <sys/select.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "TunnelLoop.h"

TunnelLoop::TunnelLoop(const std::string& tun_name, const std::string& socks5_addr, uint16_t socks5_port)
    : lwip_stack_(new LwipStack),
    tun_name_(tun_name), socks5_addr_(socks5_addr), socks5_port_(socks5_port), running_(true)
{
    lwip_stack_->init(tun_name_, socks5_addr_, socks5_port_);
}

TunnelLoop::~TunnelLoop() {
    running_ = false;
}

void TunnelLoop::run() {
    constexpr size_t buf_size = 4096;
    std::vector<uint8_t> buffer(buf_size);

    int tun_fd = lwip_stack_->getTunFd(); // 你需要在 LwipStack 提供 getTunFd() 方法

    while (running_) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(tun_fd, &readfds);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(tun_fd + 1, &readfds, nullptr, nullptr, &tv);
        if (ret < 0) {
            std::cerr << "select error\n";
            break;
        }

        if (FD_ISSET(tun_fd, &readfds)) {
            ssize_t n = lwip_stack_->read(buffer.data(), buf_size);
            if (n > 0) {
                lwip_stack_->input(buffer.data(), static_cast<size_t>(n));
            }
        }

        lwip_stack_->poll(); // 处理 lwIP 定时器和连接
    }
}
