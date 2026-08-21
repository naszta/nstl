#include "handle_raii.hpp"

#include <fcntl.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace nstl
{
#ifdef _WIN32
WindowsHandle::WindowsHandle() = default;
static_assert(WindowsHandle::invalid_value == INVALID_HANDLE_VALUE, "Invalid value must be updated");
bool WindowsHandle::valid(HANDLE hnd_) const { return hnd_ != this->invalid_value && hnd_ != nullptr; }
void WindowsHandle::free(HANDLE hnd_) const { ::CloseHandle(hnd_); }
#endif

FileIntHandle::FileIntHandle() = default;
bool FileIntHandle::valid(int hnd_) const { return 0 < hnd_; }
void FileIntHandle::free(int hnd_) const { ::close(hnd_); }
} // namespace nstl
