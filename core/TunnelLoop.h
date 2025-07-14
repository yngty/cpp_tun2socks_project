#ifndef TUNNEL_LOOP_H
#define TUNNEL_LOOP_H

#include <string>
#include <memory>
#include "LwipStack.h"

class TunnelLoop {
public:
    TunnelLoop(const std::string& tun_name, const std::string& socks5_addr, uint16_t socks5_port);
    ~TunnelLoop();

    void run();

private:
    std::unique_ptr<LwipStack> lwip_stack_;
    std::string tun_name_;
    std::string socks5_addr_;
    uint16_t socks5_port_;
    bool running_;
};


#endif
