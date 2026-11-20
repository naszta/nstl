#include "config.hpp"
#include "worker.hpp"

#include <nstl/global_init.hpp>
#include <nstl/logger.hpp>
#include <nstl/log_rotate.hpp>
#include <nstl/version.hpp>

#include <boost/program_options.hpp>

#include <iostream>
#include <memory>
#include <string>

#include <cstdlib>

namespace po = boost::program_options;

int main(int argc_, char** argv_)
{
    bool verbose = false;
    bool show_help = false;
    bool dry_run = false;
    bool show_version = false;
    std::string config_file;
    std::string log_file;

    po::options_description desc("Allowed options");
    desc.add_options()("config,c", po::value(&config_file)->required(),
                       "config file path")("logfile,l", po::value(&log_file), "log file (if not set: stdout)")(
        "dry,d", po::bool_switch(&dry_run)->implicit_value(true),
        "dry run")("verbose,v", po::bool_switch(&verbose)->implicit_value(true),
                   "Verbose logs")("help,h", po::bool_switch(&show_help)->implicit_value(true),
                                   "Show help")("version", po::bool_switch(&show_version)->implicit_value(true), "Show version");
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
    if (show_version)
    {
        std::cout << "Version: " << nstl::version() << std::endl;
        return EXIT_SUCCESS;
    }

    const nstl::global_init instance;
    const auto level = verbose ? nstl::log::level::Debug : nstl::log::level::Info;
    nstl::log_rotate(log_file);
    const auto logger = log_file.empty() ? nstl::log::Logger{ level } : nstl::log::Logger{ log_file, level };

    auto configs = nstl::ddns::parse_file(config_file);
    if (configs.empty())
    {
        std::cerr << "empty config" << std::endl;
        return EXIT_FAILURE;
    }

    nstl::ddns::worker runner{ std::move(configs), dry_run };
    runner.run(std::chrono::seconds{ 1 });
    return EXIT_SUCCESS;
}
