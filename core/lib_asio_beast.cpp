// #include <boost/asio/impl/src.hpp>
extern "C" void lwip_example_app_platform_assert(const char *msg, int line, const char *file) {
    // fprintf(stderr, "LWIP ASSERT FAILED: %s at %s:%d\n", msg, file, line);
    // abort();
}
