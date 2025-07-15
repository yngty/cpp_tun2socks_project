#ifndef TUN_DEVICE_H
#define TUN_DEVICE_H

#include <string>

class TunDevice {
public:
    TunDevice(const std::string& device);
    void init();
    void open();
    void close();
    ssize_t read(void* buffer, size_t size);
    ssize_t write(const void* buffer, size_t size);
    bool setMtu(int mtu);
    void setState(bool up);
    void setIpv4Address(const std::string& address, int prefix);
    void setIpv6Address(const std::string& address, int prefix);
    int fd() const { return fd_; }
    const std::string& getDeviceName() const { return device_; }
private:
    int fd_;
    std::string device_;
};

#endif // TUN_DEVICE_H
