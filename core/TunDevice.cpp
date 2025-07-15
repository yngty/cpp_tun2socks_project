#include "TunDevice.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
// #include <netinet6/nd6.h>
#include <net/if_utun.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <TargetConditionals.h>
namespace {
    auto fd_deleter = [](int* pfd) {
        if (pfd && *pfd >= 0) ::close(*pfd);
        delete pfd;
    };
}
#ifndef ND6_INFINITE_LIFETIME
#define ND6_INFINITE_LIFETIME 0xffffffff
#endif
TunDevice::TunDevice(const std::string& device) : device_(device), fd_(-1) {}

void TunDevice::init() {
    open();
    setMtu(8500);
    setState(true);
    setIpv4Address("192.168.0.1", 32);
    setIpv6Address("fc00::1", 128);
}
void TunDevice::open() {
     if (fd_ >= 0) {
        std::cerr << "TUN device already open: " << device_ << std::endl;
        return;
    }

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

void TunDevice::setState(bool up) {
    std::unique_ptr<int, decltype(fd_deleter)> fd_ptr(new int(socket(AF_INET, SOCK_STREAM, 0)), fd_deleter);
    if (*fd_ptr < 0) {
        std::cerr << "Failed to create socket for setting state: " << device_ << std::endl;
        throw std::runtime_error("Socket creation failed");
    }

    struct ifreq ifr = {};
    strncpy(ifr.ifr_name, device_.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(*fd_ptr, SIOCGIFFLAGS, &ifr) < 0) {
        std::cerr << "Failed to get flags for TUN device: " << device_ << std::endl;
        throw std::runtime_error("Getting TUN device flags failed");
    }
    if (up) {
        ifr.ifr_flags |= IFF_UP;
    } else {
        ifr.ifr_flags &= ~IFF_UP;
    }
    if (ioctl(*fd_ptr, SIOCSIFFLAGS, &ifr) < 0) {
        std::cerr << "Failed to set flags for TUN device: " << device_ << std::endl;
        throw std::runtime_error("Setting TUN device flags failed");
    }
}

bool TunDevice::setMtu(int mtu) {
    std::unique_ptr<int, decltype(fd_deleter)> fd_ptr(new int(socket(AF_INET, SOCK_STREAM, 0)), fd_deleter);

    if (*fd_ptr < 0) {
        std::cerr << "Failed to create socket for setting MTU: " << device_ << std::endl;
        return false;
    }

    struct ifreq ifr = { .ifr_mtu = mtu };
    strncpy(ifr.ifr_name, device_.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(*fd_ptr, SIOCSIFMTU, &ifr) < 0) {
        std::cerr << "Failed to set MTU for TUN device: " << device_ << std::endl;

        return false;
    }
    return true;
}

void TunDevice::setIpv4Address(const std::string& address, int prefix) {
    std::unique_ptr<int, decltype(fd_deleter)> fd_ptr(new int(socket(AF_INET, SOCK_DGRAM, 0)), fd_deleter);
    if (*fd_ptr < 0) {
        std::cerr << "Failed to create socket for setting IPv4 address: " << device_ << std::endl;
        throw std::runtime_error("Socket creation failed");
    }

    struct ifaliasreq ifra = {};
    strncpy(ifra.ifra_name, device_.c_str(), IFNAMSIZ - 1);
    ifra.ifra_name[IFNAMSIZ - 1] = '\0';

    struct sockaddr_in* pa = (struct sockaddr_in *)&ifra.ifra_addr;
    pa->sin_family = AF_INET;
    pa->sin_len = sizeof(struct sockaddr_in);
    int res = inet_pton(AF_INET, address.c_str(), &pa->sin_addr);
    if (res <= 0) {
        std::cerr << "Invalid IPv4 address format: " << address << std::endl;
        throw std::runtime_error("Invalid IPv4 address");
    }

    memcpy (&ifra.ifra_broadaddr, &ifra.ifra_addr, sizeof (ifra.ifra_addr));

    pa = (struct sockaddr_in *)&ifra.ifra_mask;
    pa->sin_len = sizeof (ifra.ifra_addr);
    pa->sin_family = AF_INET;
    pa->sin_addr.s_addr = htonl (((unsigned int)(-1)) << (32 - prefix));

    if (ioctl(*fd_ptr, SIOCAIFADDR, &ifra) < 0) {
        std::cerr << "Failed to set IPv4 address for TUN device: " << device_ << std::endl;
        throw std::runtime_error("Setting IPv4 address failed");
    }
}

void TunDevice::setIpv6Address(const std::string& address, int prefix) {
    std::unique_ptr<int, decltype(fd_deleter)> fd_ptr(new int(socket(AF_INET6, SOCK_STREAM, 0)), fd_deleter);
    if (*fd_ptr < 0) {
        std::cerr << "Failed to create socket for setting IPv6 address: " << device_ << std::endl;
        throw std::runtime_error("Socket creation failed");
    }

    struct in6_aliasreq ifra = {};
    ifra.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
    ifra.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;

    strncpy(ifra.ifra_name, device_.c_str(), IFNAMSIZ - 1);
    ifra.ifra_name[IFNAMSIZ - 1] = '\0';

    ifra.ifra_addr.sin6_len = sizeof (ifra.ifra_addr);
    ifra.ifra_addr.sin6_family = AF_INET6;
    if (!inet_pton (AF_INET6, address.c_str(), &ifra.ifra_addr.sin6_addr)) {
        std::cerr << "Invalid IPv6 address format: " << address << std::endl;
        throw std::runtime_error("Invalid IPv6 address");
    }

    ifra.ifra_prefixmask.sin6_len = sizeof (ifra.ifra_prefixmask);
    ifra.ifra_prefixmask.sin6_family = AF_INET6;
    uint8_t* bytes = (uint8_t *)&ifra.ifra_prefixmask.sin6_addr;
    memset (bytes, 0xFF, 16);
    bytes[prefix / 8] <<= prefix % 8;
    prefix += prefix % 8;
    for (int i = prefix / 8; i < 16; i++)
        bytes[i] = 0;

    if (ioctl(*fd_ptr, SIOCAIFADDR_IN6, &ifra) < 0) {
        std::cerr << "Failed to set IPv6 address for TUN device: " << device_ << std::endl;
        throw std::runtime_error("Setting IPv6 address failed");
    }

}
