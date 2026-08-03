#ifndef _NSTL_GLOBAL_INIT
#define _NSTL_GLOBAL_INIT 1

namespace nstl
{
class global_init
{
    const bool _curl_init{true};

public:
    explicit global_init(bool curl_init_ = true);
    ~global_init();

    global_init(const global_init&) = delete;
    global_init& operator=(const global_init&) = delete;
};
}

#endif
