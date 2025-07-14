#include "LwipStack.h"
#include "Connection.h"
#include <iostream>
#include <cstring>
extern "C" {
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
}

static err_t tunif_output(struct pbuf *p, struct netif *inp) {
    // 获取 LwipStack 实例
    LwipStack* stack = static_cast<LwipStack*>(inp->state);
    if (!stack || !stack->isTunDeviceOpen()) return ERR_IF;

    std::vector<uint8_t> buf(p->tot_len);
    pbuf_copy_partial(p, buf.data(), buf.size(), 0);
    ssize_t n = stack->write(buf.data(), buf.size());
    return (n == (ssize_t)buf.size()) ? ERR_OK : ERR_IF;
}

static err_t tcp_accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || !newpcb) return ERR_VAL;
    // 获取 LwipStack 实例
    LwipStack* stack = static_cast<LwipStack*>(arg);
    if (stack) {
        ip_addr_t remote_ip = newpcb->remote_ip;
        uint16_t remote_port = newpcb->remote_port;
        std::string ip_str = ipaddr_ntoa(&remote_ip);
        stack->onNewTcpConnection(ip_str, remote_port);
    }
    return ERR_OK;
}

LwipStack::LwipStack() : tunDevice_(nullptr) {
    std::memset(&netif_, 0, sizeof(netif_));
}

LwipStack::~LwipStack() {
    stop();
}

void LwipStack::init(const std::string& tun_name, const std::string& socks5_addr, uint16_t socks5_port) {
    // 1. 初始化 lwIP 协议栈
    lwip_init();

    // 2. 打开 TUN 设备
    if (!tunDevice_) {
        tunDevice_ = std::make_unique<TunDevice>(tun_name);
        tunDevice_->open();
    }

    // 3. 配置 netif
    ip4_addr_t ipaddr, netmask, gw;
    IP4_ADDR(&ipaddr, 10,0,0,2);
    IP4_ADDR(&netmask, 255,255,255,0);
    IP4_ADDR(&gw, 10,0,0,1);

    netif_add(&netif_, &ipaddr, &netmask, &gw, this, nullptr, tunif_output);
    netif_.state = this;
    netif_set_default(&netif_);
    netif_set_up(&netif_);

    // 4. 注册 TCP 监听和 accept 回调
    struct tcp_pcb* listen_pcb = tcp_new();
    tcp_bind(listen_pcb, IP_ADDR_ANY, 0); // 监听任意端口
    listen_pcb = tcp_listen(listen_pcb);
    tcp_accept(listen_pcb, tcp_accept_cb);
    listen_pcb->callback_arg = this;

    std::cout << "[LwipStack] lwIP initialized and bound to TUN device: " << tunDevice_->getDeviceName() << std::endl;
}

void LwipStack::input(const uint8_t* data, size_t len) {
    struct pbuf* p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (!p) return;
    pbuf_take(p, data, len);
    if (netif_.input(p, &netif_) != ERR_OK) {
        pbuf_free(p);
    }
}

void LwipStack::start() {
    if (!tunDevice_) {
        std::cerr << "TUN device not initialized." << std::endl;
        return;
    }
    std::cout << "LwIP stack started with TUN device: " << tunDevice_->getDeviceName() << std::endl;
}

void LwipStack::stop() {
    if (tunDevice_) {
        tunDevice_->close();
        tunDevice_.reset();
        std::cout << "LwIP stack stopped." << std::endl;
    } else {
        std::cerr << "TUN device not initialized, nothing to stop." << std::endl;
    }
}

void LwipStack::poll() {
    // 这里可以添加轮询逻辑，处理 lwIP 事件
    // 例如调用 netif_poll() 或其他相关函数
    // lwIP 的定时器和事件处理通常在主循环中调用
    // sys_check_timeouts();
}
void LwipStack::onNewTcpConnection(const std::string& remote_ip, uint16_t remote_port) {
    auto conn = std::make_shared<Connection>(remote_ip, remote_port, socks5_host_, socks5_port_);
    connections_.push_back(conn);
    conn->start();
}
