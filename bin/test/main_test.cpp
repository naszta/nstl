#include <cstdlib>

#include <nstl/args_editor.hpp>
#include <nstl/global_init.hpp>
#include <nstl/logging.hpp>
#include <nstl/range_print.hpp>
#include <nstl/signal_barrier.hpp>
#include <nstl/env_var_raii.hpp>

int main(int argc_, char** argv_)
{
    const nstl::global_init init{true};
    nstl::log::Logger logger;
    const auto pr_id = nstl::get_env_var("NSTL_PROCESSES_ID");
    const auto pr_num = nstl::get_env_var("NSTL_PROCESSES_NUMBER");
    const auto healthy = nstl::is_arg_set(argc_, const_cast<const char**>(argv_), "--healthy");

    NSTL_INFO("NSTL_PROCESSES_ID: " << pr_id << "; NSTL_PROCESSES_NUMBER: " << pr_num << " (args: " << nstl::range_print_iter(argv_, argv_ + argc_, ' ') << ')');

    nstl::SignalBarrier barrier;
    const auto value = barrier.wait();
    NSTL_INFO(value << " received");
    return healthy ? EXIT_SUCCESS : nstl::to_signal_conv(value);
}
