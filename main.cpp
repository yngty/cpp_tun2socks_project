#include "core/TunnelLoop.h"

int main(int argc, char* argv[]) {
    TunnelLoop loop("hev0", "127.0.0.1", 1080);
    loop.run();
    return 0;
}
