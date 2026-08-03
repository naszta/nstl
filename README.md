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
- Windows: links against `Ws2_32.lib`, `Dnsapi.lib`, and `Advapi32.lib` (hashing via CryptoAPI)
- Linux/macOS: links against `resolv`, and requires [OpenSSL](https://www.openssl.org/) (found via `find_package(OpenSSL REQUIRED)`, e.g. `apt install libssl-dev`) for hashing
- macOS: builds the [HowardHinnant/date](https://github.com/HowardHinnant/date) library from the `modules/date` git submodule, since Apple's `std::chrono::time_zone` support isn't complete
- [libcurl](https://curl.se/libcurl/) — required unless configured with `-DNSTL_USING_CURL=OFF`, used by the HTTP client. On Windows it's fetched and built from source (static, Schannel-backed) via `FetchContent`; elsewhere it's found via `find_package(CURL COMPONENTS HTTPS SSL)` (e.g. `apt install libcurl4-openssl-dev`)

## Building

```sh
git submodule update --init --recursive   # only needed on macOS, for the HH date library
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
| [`nstl/c_timezone.hpp`](nstl/nstl/c_timezone.hpp) | `nstl::c_timezone` — converts between local and system time using the platform C library (works around incomplete `std::chrono::time_zone` support on some standard library implementations). On macOS it's built on the [HowardHinnant/date](https://github.com/HowardHinnant/date) library instead of `std::chrono` directly. Also exposes `nstl::time::localtime_r` / `gmtime_r` / `timegm` / `mktime` — thread-safe, cross-platform wrappers over the C time API (`*_s` on Windows, `*_r` on POSIX). |
| [`nstl/dns_tools.hpp`](nstl/nstl/dns_tools.hpp) | `nstl::net` — small DNS helpers: `hostname()`, `canonical_name()`, `mx_name()`, `txt_name()`, `c_name()` (each overloaded for `const char*` and `std::string`). Backed by native resolver APIs (Windows DNS API / POSIX `resolv`). |
| [`nstl/ro_buffer.hpp`](nstl/nstl/ro_buffer.hpp) | `nstl::basic_ro_buffer` — a read-only `std::streambuf` wrapping an existing `char`/`wchar_t` buffer, `string_view`, or `span`, so you can build an `istream` over memory without copying. |
| [`nstl/range_print.hpp`](nstl/nstl/range_print.hpp) | `range_print` / `range_map_print` — stream a range (or map-like range) to an `ostream` with a custom delimiter, without manually looping. |
| [`nstl/scope_exit.hpp`](nstl/nstl/scope_exit.hpp) | `nstl::scope_exit` / `on_scope_exit` — RAII guard that runs a callable when the scope exits. |
| [`nstl/unlock_guard.hpp`](nstl/nstl/unlock_guard.hpp) | `nstl::unlock_guard` — the inverse of `std::lock_guard`: unlocks one or more mutexes for the current scope and re-locks them on destruction. |
| [`nstl/safe_basename.hpp`](nstl/nstl/safe_basename.hpp) | `nstl::safe_basename` / `nstl::safe_basename_view` — returns the filename portion of a path (as a `const char*` or `constexpr string_view`), platform-aware (`\\` on Windows, `/` elsewhere), without allocating. |
| [`nstl/string.hpp`](nstl/nstl/string.hpp) | `nstl::split_view_func`, `nstl::trim_view` / `left_trim_view` / `right_trim_view` — header-only `string_view` helpers to split on a delimiter (invoking a callback per token) and trim leading/trailing whitespace, without allocating. |
| [`nstl/temp_dir.hpp`](nstl/nstl/temp_dir.hpp) | `nstl::temp_dir` — RAII wrapper that creates a temporary directory (random, named, or under a given parent) and removes it on destruction. |
| [`nstl/macros.hpp`](nstl/nstl/macros.hpp) | `NSTL_THROW_EXCEPTION` / `NSTL_THROW_EXCEPTION_IF` — throw an exception of a caller-chosen type, with a message prefixed by file (via `safe_basename_view`) and line number. |
| [`nstl/exception.hpp`](nstl/nstl/exception.hpp) | `nstl::exception` — a `std::runtime_error` that records the throw site's file/line and prints nested exception chains via `operator<<`. The `NSTL2_THROW_EXCEPTION` / `NSTL2_THROW_EXCEPTION_IF` macros throw one with a message prefixed by file (via `safe_basename_view`) and line number; `NSTL2_NESTED_THROW_EXCEPTION` does the same via `std::throw_with_nested`, to chain onto a caught exception. |
| [`nstl/env_var_raii.hpp`](nstl/nstl/env_var_raii.hpp) | `nstl::env_var_raii` — RAII guard that sets (or clears) an environment variable and restores its previous value on destruction. `get_env_var()` reads one without throwing if unset. |
| [`nstl/logging.hpp`](nstl/nstl/logging.hpp) | `nstl::log::Logger` and the `NSTL_DEBUG` / `NSTL_INFO` / `NSTL_WARNING` / `NSTL_ERROR` / `NSTL_TERMINATE` macros — leveled, timestamped logging to a file, `ostream`, or a custom sink function, with a timezone-aware timestamp via `LogTimeZone`. Log lines are handed off to a background thread through a TBB concurrent queue, so callers don't block on I/O. Constructing a `Logger` pushes it onto a stack (only one console logger may be active at a time), so the innermost one receives log calls and the previous one resumes when it's destroyed; `throttleSize()` caps the queue so a stalled sink can't grow unbounded; `NSTL_TERMINATE` logs, flushes, and calls `std::abort()`. |
| [`nstl/datahash.hpp`](nstl/nstl/datahash.hpp) / [`nstl/datahash_fwd.hpp`](nstl/nstl/datahash_fwd.hpp) | `nstl::Hasher` — a streaming MD5/SHA1/SHA256/SHA512 hasher (`HashType`) whose `add()` takes raw bytes, a `span` of any trivially-copyable type, a `string_view`, or a NUL-terminated `char*`/`wchar_t*`. `hash_file()` hashes a file directly (optionally into a caller-supplied buffer), and `hash_to_hex()` renders a readable digest. Backed by OpenSSL on Linux/macOS and Windows CryptoAPI on Windows. |
| [`nstl/global_init.hpp`](nstl/nstl/global_init.hpp) | `nstl::global_init` — RAII one-time process-wide setup/teardown: Winsock (`WSAStartup`/`WSACleanup`) on Windows, plus `curl_global_init`/`curl_global_cleanup` when the HTTP client is enabled. Construct one instance before using networking components. |
| [`nstl/http_client.hpp`](nstl/nstl/http_client.hpp) | `nstl::HttpClient` — a libcurl-based HTTP client: `get()` / `post()`, `add_header()`, `url_encode()` / `url_decode()`, and the static helpers `is_http_success()`, `is_valid_url()`, `is_ssl_supported()`. Requires `nstl::global_init` to have run first; only built when `NSTL_USING_CURL` is on (the default). |

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
