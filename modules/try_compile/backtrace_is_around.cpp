#include <backtrace.h>

#include <iostream>
#include <cstdlib>

backtrace_state* bt_state{ nullptr };

namespace
{
void bt_error_cb(void* /*data*/, const char* msg, int errnum)
{
    std::cerr << "BT error (" << errnum << "): " << (msg ? msg : "") << std::endl;
}

void backtrace_init()
{
    if (!bt_state)
    {
        bt_state = ::backtrace_create_state(nullptr, 1, bt_error_cb, nullptr);
    }
}
} // namespace

int main(int, char**)
{
    backtrace_init();
    std::cout << std::hex << reinterpret_cast<std::ptrdiff_t>(bt_state) << std::endl;
    return EXIT_SUCCESS;
}
