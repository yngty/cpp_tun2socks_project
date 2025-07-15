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
static err_t netif_output_handler (struct netif *netif, struct pbuf *p)
{
    std::cout << "[LwipStack] netif_output_handler called." << std::endl;

    if (!p || p->tot_len == 0) {
        std::cerr << "[LwipStack] Invalid pbuf or length is zero." << std::endl;
        return ERR_BUF;
    }

    LwipStack* stack = static_cast<LwipStack*>(netif->state);
    if (!stack || !stack->isTunDeviceOpen()) {
        std::cerr << "[LwipStack] TUN device is not open or stack is null." << std::endl;
        return ERR_IF;
    }

    std::vector<uint8_t> buf(p->tot_len);
    pbuf_copy_partial(p, buf.data(), buf.size(), 0);
    ssize_t n = stack->write(buf.data(), buf.size());
    return (n == (ssize_t)buf.size()) ? ERR_OK : ERR_IF;
}
static err_t netif_output_v4_handler (struct netif *netif, struct pbuf *p,
                         const ip4_addr_t *ipaddr)
{
    return netif_output_handler (netif, p);
}

static err_t netif_output_v6_handler (struct netif *netif, struct pbuf *p,
                         const ip6_addr_t *ipaddr)
{
    return netif_output_handler (netif, p);
}

static err_t netif_init_handler (struct netif *netif)
{
    std::cout << "[LwipStack] netif_init_handler called." << std::endl;
    netif->output = netif_output_v4_handler;
    netif->output_ip6 = netif_output_v6_handler;

    return ERR_OK;
}

static err_t tcp_accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || !newpcb) {
        std::cerr << "[LwipStack] TCP accept callback error: " << lwip_strerr(err) << std::endl;
        return err; // 返回错误
    }
    std::cout << "[LwipStack] New TCP connection accepted." << std::endl;
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
    // std::memset(&netif_, 0, sizeof(netif_));
}

LwipStack::~LwipStack() {
    stop();
}

void LwipStack::init(const std::string& tun_name, const std::string& socks5_addr, uint16_t socks5_port) {
    // 1. 初始化 lwIP 协议栈
    lwip_init();

    // 2. 打开 TUN 设备
    if (!tunDevice_) {
        tunDevice_ = std::unique_ptr<TunDevice>(new TunDevice(tun_name));
        tunDevice_->init();
    }

    // 3. 配置 netif
    ip4_addr_t addr4, mask, gw;
    ip6_addr_t addr6;

    netif_add_noaddr (&netif_, nullptr, netif_init_handler, ip_input);

    ip4_addr_set_loopback (&addr4);
    ip4_addr_set_any (&mask);
    ip4_addr_set_any (&gw);
    netif_set_addr (&netif_, &addr4, &mask, &gw);

    ip6_addr_set_loopback (&addr6);
    netif_add_ip6_address (&netif_, &addr6, nullptr);

    netif_set_up (&netif_);
    netif_set_link_up (&netif_);
    netif_set_default (&netif_);
    // netif_set_flags (&netif_, NETIF_FLAG_PRETEND_TCP);

    // 4. 注册 TCP 监听和 accept 回调
    tcp_ = tcp_new_ip_type (IPADDR_TYPE_ANY);
    tcp_bind_netif (tcp_, &netif_);
    err_t res = tcp_bind(tcp_, nullptr, 0);
    if (res != ERR_OK) {
        std::cerr << "[LwipStack] Failed to bind TCP PCB: " << lwip_strerr(res) << std::endl;
        return;
    }
    tcp_ = tcp_listen (tcp_);
    tcp_accept (tcp_, tcp_accept_cb);
    tcp_->callback_arg = this;

    std::cout << "[LwipStack] lwIP initialized and bound to TUN device: " << tunDevice_->getDeviceName() << std::endl;
}

void LwipStack::input(const uint8_t* data, size_t len) {
    if (!data || len == 0) {
        std::cerr << "[LwipStack] Invalid input data or length is zero." << std::endl;
        return;
    }
    std::cout << "[LwipStack] Input data of length: " << len << std::endl;

    struct pbuf* p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    if (!p) return;
    pbuf_take(p, data, len);
    if (netif_.input(p, &netif_) != ERR_OK) {
        pbuf_free(p);
    } else {
        std::cout << "[LwipStack] Data input processed successfully." << std::endl;
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

ssize_t LwipStack::write(const uint8_t* data, size_t len) {
    if (!tunDevice_ || !isTunDeviceOpen()) {
        std::cerr << "TUN device is not open." << std::endl;
        return -1;
    }
    if (!data || len == 0) {
        std::cerr << "[LwipStack] Invalid write data or length is zero." << std::endl;
        return -1;
    }
    std::cout << "[LwipStack] Writing data of length: " << len << std::endl;
    return tunDevice_->write(data, len);
}

ssize_t LwipStack::read(void* buffer, size_t size) {
    if (!tunDevice_ || !isTunDeviceOpen()) {
        std::cerr << "TUN device is not open." << std::endl;
        return -1;
    }
    if (!buffer || size == 0) {
        std::cerr << "[LwipStack] Invalid read buffer or size is zero." << std::endl;
        return -1;
    }
    return tunDevice_->read(buffer, size);
}

void LwipStack::poll() {

}
void LwipStack::onNewTcpConnection(const std::string& remote_ip, uint16_t remote_port) {
    if (remote_ip.empty() || remote_port == 0) {
        std::cerr << "[LwipStack] Invalid remote IP or port." << std::endl;
        return;
    }
    std::cout << "[LwipStack] New TCP connection from " << remote_ip << ":" << remote_port << std::endl;

    auto conn = std::make_shared<Connection>(remote_ip, remote_port, socks5_host_, socks5_port_);
    connections_.push_back(conn);
    conn->start();
}
