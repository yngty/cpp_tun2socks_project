#include "TunDevice.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <unistd.h>

TunDevice::TunDevice(const std::string& device) : device_(device), fd_(-1) {}

void TunDevice::open() {
    if (fd_ >= 0) return; // 已打开

    fd_ = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd_ < 0) {
        std::cerr << "Failed to create socket for TUN device: " << device_ << std::endl;
        throw std::runtime_error("Socket creation failed");
    }

    struct ctl_info ctlInfo = {};
    strncpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name) - 1);
    if (ioctl(fd_, CTLIOCGINFO, &ctlInfo) < 0) {
        std::cerr << "Failed to get TUN device info: " << device_ << std::endl;
        close();
        throw std::runtime_error("TUN device info retrieval failed");
    }

    struct sockaddr_ctl addr = {};
    addr.sc_len = sizeof(addr);
    addr.sc_family = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    addr.sc_id = ctlInfo.ctl_id;

    unsigned int unit = 0;
    int res = sscanf(device_.c_str(), "utun%u", &unit);
    if (res > 0)
        addr.sc_unit = unit + 1;
    else
        addr.sc_unit = 0;

    if (connect(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to TUN device: " << device_ << std::endl;
        close();
        throw std::runtime_error("TUN device connection failed");
    }
    int nonblock = 1;
    if (ioctl(fd_, FIONBIO, &nonblock) < 0) {
        std::cerr << "Failed to set TUN device to non-blocking mode: " << device_ << std::endl;
        close();
        throw std::runtime_error("Setting non-blocking mode failed");
    }
    char ifname[IFNAMSIZ] = {0};
    socklen_t ifname_len = sizeof(ifname);
    if (getsockopt(fd_, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifname, &ifname_len) < 0) {
        std::cerr << "Failed to get TUN device name: " << device_ << std::endl;
        close();
        throw std::runtime_error("Getting TUN device name failed");
    }
    device_ = ifname;
    std::cout << "Opened TUN device: " << device_ << std::endl;
}

void TunDevice::close() {
    std::cout << "Closing TUN device: " << device_ << std::endl;
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

ssize_t TunDevice::read(void* buffer, size_t size) {
    return ::read(fd_, buffer, size);
}
ssize_t TunDevice::write(const void* buffer, size_t size) {
    return ::write(fd_, buffer, size);
}
