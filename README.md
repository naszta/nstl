# nstl

A lightweight, header-first C++20 extension library. `nstl` fills in small,
commonly needed gaps around the standard library — timezone conversion, DNS
lookups, read-only stream buffers, scope guards, and a few other utilities —
without pulling in a heavyweight dependency.

## Requirements

- C++20 compiler (GCC, Clang, or MSVC)
- CMake 3.28+
- [oneTBB](https://github.com/uxlfoundation/oneTBB) (found via `find_package(TBB REQUIRED)`) — used for the async logging queue
- [GoogleTest](https://github.com/google/googletest) (found via `find_package(GTest REQUIRED)`) — install it via your system/package manager (e.g. `apt install libgtest-dev googletest`), it is no longer fetched automatically
- Windows: links against `Ws2_32.lib` and `Dnsapi.lib`
- Linux/macOS: links against `resolv`

## Building

```sh
cmake -S . -B build
cmake --build build
```

Tests (GoogleTest, found on the system via `find_package`) run through CTest:

```sh
ctest --test-dir build
```

A `Dockerfile` is provided for a reproducible Ubuntu-based build:

```sh
docker build -t nstl .
```

## Components

| Header | Description |
| --- | --- |
| [`nstl/c_timezone.hpp`](nstl/nstl/c_timezone.hpp) | `nstl::c_timezone` — converts between `std::chrono` local and system time using the platform C library (works around incomplete `std::chrono::time_zone` support on some standard library implementations). |
| [`nstl/dns_tools.hpp`](nstl/nstl/dns_tools.hpp) | `nstl::net` — small DNS helpers: `hostname()`, `cannonical_name()`, `mx_name()`, `txt_name()`, `c_name()`. Backed by native resolver APIs (Windows DNS API / POSIX `resolv`). |
| [`nstl/ro_buffer.hpp`](nstl/nstl/ro_buffer.hpp) | `nstl::basic_ro_buffer` — a read-only `std::streambuf` wrapping an existing `char`/`wchar_t` buffer, `string_view`, or `span`, so you can build an `istream` over memory without copying. |
| [`nstl/range_print.hpp`](nstl/nstl/range_print.hpp) | `range_print` / `range_map_print` — stream a range (or map-like range) to an `ostream` with a custom delimiter, without manually looping. |
| [`nstl/scope_exit.hpp`](nstl/nstl/scope_exit.hpp) | `nstl::scope_exit` / `on_scope_exit` — RAII guard that runs a callable when the scope exits. |
| [`nstl/unlock_guard.hpp`](nstl/nstl/unlock_guard.hpp) | `nstl::unlock_guard` — the inverse of `std::lock_guard`: unlocks one or more mutexes for the current scope and re-locks them on destruction. |
| [`nstl/safe_basename.hpp`](nstl/nstl/safe_basename.hpp) | `nstl::safe_basename` — returns the filename portion of a path, platform-aware (`\\` on Windows, `/` elsewhere), without allocating. |
| [`nstl/string.hpp`](nstl/nstl/string.hpp) | `nstl::split_view_func`, `nstl::trim_view` / `left_trim_view` / `right_trim_view` — header-only `string_view` helpers to split on a delimiter (invoking a callback per token) and trim leading/trailing whitespace, without allocating. |
| [`nstl/temp_dir.hpp`](nstl/nstl/temp_dir.hpp) | `nstl::temp_dir` — RAII wrapper that creates a temporary directory (random, named, or under a given parent) and removes it on destruction. |
| [`nstl/macros.hpp`](nstl/nstl/macros.hpp) | `NSTL_THROW_EXCEPTION` / `NSTL_THROW_EXCEPTION_IF` — throw an exception of a caller-chosen type, with a message prefixed by file (via `safe_basename`) and line number. |
| [`nstl/exception.hpp`](nstl/nstl/exception.hpp) | `nstl::exception` — a `std::runtime_error` that records the throw site's file/line and prints nested exception chains via `operator<<`. The `NSTL2_THROW_EXCEPTION` / `NSTL2_THROW_EXCEPTION_IF` macros throw one with a message prefixed by file (via `safe_basename`) and line number. |
| [`nstl/logging.hpp`](nstl/nstl/logging.hpp) | `nstl::log::Logger` and the `NSTL_DEBUG` / `NSTL_INFO` / `NSTL_WARNING` / `NSTL_ERROR` macros — leveled, timestamped logging to a file, `ostream`, or a custom sink function, with a timezone-aware timestamp via `LogTimeZone`. Log lines are handed off to a background thread through a TBB concurrent queue, so callers don't block on I/O. |

## Usage

Add `nstl` as a subdirectory or dependency and link against the `nstllib`
CMake target:

```cmake
add_subdirectory(nstl)
target_link_libraries(your_target PRIVATE nstllib)
```

Then include the headers you need, e.g.:

```cpp
#include <nstl/scope_exit.hpp>

auto guard = nstl::on_scope_exit([] { cleanup(); });
```

Logging:

```cpp
#include <nstl/logging.hpp>

nstl::log::Logger logger{ std::filesystem::path{"app.log"}, nstl::log::LogLevel::Info };

NSTL_INFO("Started with " << argc << " arguments");
NSTL_ERROR("Failed to open " << path);
```

## License

MIT — see [LICENSE](LICENSE).
