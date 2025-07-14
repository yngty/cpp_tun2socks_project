#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include <string>

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(const std::string& remote_ip, uint16_t remote_port, const std::string& socks5_host, uint16_t socks5_port);
    void start();
    void stop();
private:
    std::string remote_ip_;
    uint16_t remote_port_;
    std::string socks5_host_;
    uint16_t socks5_port_;
};

#endif // CONNECTION_H
