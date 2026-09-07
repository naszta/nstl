#include <cstdlib>

#include <chrono>
#include <format>
#include <iterator>
#include <iostream>

int main(int, char**)
{
    const auto now = std::chrono::system_clock::now();
    const auto tzone = std::chrono::locate_zone("Europe/London");
    const std::chrono::zoned_time zt{tzone, now};
    std::cout << "Now: ";
    std::format_to(std::ostream_iterator<char>{ std::cout }, "{:%FT%T%z}", zt);
    std::cout << std::endl;
    return EXIT_SUCCESS;
}
