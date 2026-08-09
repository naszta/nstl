#include <nstl/global_init.hpp>
#include <nstl/logging.hpp>

#include <boost/program_options.hpp>
#include <gtest/gtest.h>

#include <cstdlib>

namespace po = boost::program_options;

int main(int argc_, char** argv_)
{
    ::testing::InitGoogleTest(&argc_, argv_);

    bool show_help = false;
    bool verbose = false;
    bool pass_tests = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("verbose,v", po::bool_switch(&verbose)->implicit_value(true), "Verbose")
        ("help,h", po::bool_switch(&show_help)->implicit_value(true), "Show help")
        ("pass,p", po::bool_switch(&pass_tests)->implicit_value(true), "Pass the tests");

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc_, argv_, desc), vm);
        po::notify(vm);
    }
    catch (const std::exception& exc_)
    {
        std::cerr << "Failed argument processing:\n" << exc_.what() << "\n" << std::endl;
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }
    if (show_help)
    {
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }

    const nstl::global_init instance{ true };
    nstl::log::Logger logger{ verbose ? nstl::log::LogLevel::Debug : nstl::log::LogLevel::Info };
    NSTL_INFO("Log working");
    return RUN_ALL_TESTS();
}
