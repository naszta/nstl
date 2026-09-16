#include <cstdlib>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <iostream>

int main(int argc_, char** argv_)
{
    bool show_help = false;
    std::string domain_name;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("domain,d", po::value(&domain_name)->required(), "Checked domain (required)")
        ("help,h", po::bool_switch(&show_help)->implicit_value(true), "Show help");

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

	return EXIT_SUCCESS;
}