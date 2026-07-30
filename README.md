# nstl

A lightweight, header-first C++20 extension library. `nstl` fills in small,
commonly needed gaps around the standard library — timezone conversion, DNS
lookups, an HTTP client, file hashing, structured logging, deadlock detection,
and a handful of RAII helpers (scope guards, temp dirs, env vars) — without
pulling in a heavyweight dependency.

## Requirements

- C++20 compiler (GCC, Clang, or MSVC)
- CMake 3.28+
- [oneTBB](https://github.com/uxlfoundation/oneTBB) — used for the async logging queue. On Linux it's found via `find_package(TBB REQUIRED)` (e.g. `apt install libtbb-dev`); on Windows/macOS it's fetched and built from source via `FetchContent`
- [GoogleTest](https://github.com/google/googletest) — on Linux it's found via `find_package(GTest REQUIRED)` (install via your system/package manager, e.g. `apt install libgtest-dev googletest`); on Windows/macOS it's fetched automatically via `FetchContent`
- [Boost](https://www.boost.org/) (`headers`, `serialization`, and `program_options` components) — `headers` (Boost.Archive's base64 iterators) backs `base64.hpp`. On Linux it's found via `find_package(Boost COMPONENTS headers serialization program_options)` (e.g. `apt install libboost-all-dev`); on Windows/macOS it's fetched via `FetchContent`
- Windows: links against `Ws2_32.lib`, `Dnsapi.lib`, `Advapi32.lib` (hashing via CryptoAPI), and `dbghelp.lib` (symbolized stack traces)
- Linux: links against `resolv` and `backtrace` (libbacktrace, typically bundled with GCC — provides symbolized stack traces), and requires [OpenSSL](https://www.openssl.org/) (found via `find_package(OpenSSL REQUIRED)`, e.g. `apt install libssl-dev`) for hashing
- macOS: links against `resolv`, requires OpenSSL for hashing, and builds the [HowardHinnant/date](https://github.com/HowardHinnant/date) library from the `modules/date` git submodule, since Apple's `std::chrono::time_zone` support isn't complete; stack traces (`backtrace.hpp`) are a no-op stub on this platform
- [libcurl](https://curl.se/libcurl/) — required, used by the HTTP client. On Windows it's fetched and built from source (static, Schannel-backed) via `FetchContent`; elsewhere it's found via `find_package(CURL COMPONENTS HTTPS SSL)` (e.g. `apt install libcurl4-openssl-dev`)

## Building

```sh
git submodule update --init --recursive   # only needed on macOS, for the HH date library
cmake -S . -B build
cmake --build build
```

Tests (GoogleTest) run through CTest:

```sh
ctest --test-dir build
```

A `Dockerfile` is provided for a reproducible Ubuntu-based build:

```sh
docker build -t nstl .
```

`Dockerfile.windows` provides the equivalent for a Windows container (MSVC Build Tools + vcpkg for TBB/GoogleTest), and runs in CI on `windows-latest`:

```sh
docker build -t nstl-windows -f Dockerfile.windows .
```

## Components

| Header | Description |
| --- | --- |
| [`nstl/c_timezone.hpp`](nstl/nstl/c_timezone.hpp) | `nstl::c_timezone` — converts between local and system time using the platform C library (works around incomplete `std::chrono::time_zone` support on some standard library implementations). On macOS it's built on the [HowardHinnant/date](https://github.com/HowardHinnant/date) library instead of `std::chrono` directly. Also exposes `nstl::time::localtime_r` / `gmtime_r` / `timegm` / `mktime` — thread-safe, cross-platform wrappers over the C time API (`*_s` on Windows, `*_r` on POSIX). |
| [`nstl/dns_tools.hpp`](nstl/nstl/dns_tools.hpp) | `nstl::net` — small DNS helpers: `hostname()`, `canonical_name()`, `mx_name()`, `txt_name()`, `c_name()`, `srv_name()` (each overloaded for `const char*` and `std::string`; `srv_name()` returns `gen_srv` records with address/port/priority/weight). Backed by native resolver APIs (Windows DNS API / POSIX `resolv`). |
| [`nstl/ro_buffer.hpp`](nstl/nstl/ro_buffer.hpp) | `nstl::basic_ro_buffer` — a read-only `std::streambuf` wrapping an existing `char`/`wchar_t` buffer, `string_view`, or `span`, so you can build an `istream` over memory without copying. |
| [`nstl/range_print.hpp`](nstl/nstl/range_print.hpp) | `range_print` / `range_map_print` — stream a range (or map-like range) to an `ostream` with a custom delimiter, without manually looping. |
| [`nstl/scope_exit.hpp`](nstl/nstl/scope_exit.hpp) | `nstl::scope_exit` / `on_scope_exit` — RAII guard that runs a callable when the scope exits; `reset()` cancels it, `swap()` exchanges its callable, and `empty()` / `explicit operator bool()` report whether one is set. |
| [`nstl/unlock_guard.hpp`](nstl/nstl/unlock_guard.hpp) | `nstl::unlock_guard` — the inverse of `std::lock_guard`: unlocks one or more mutexes for the current scope and re-locks them on destruction. |
| [`nstl/dead_lock_detect.hpp`](nstl/nstl/dead_lock_detect.hpp) | `nstl::DeadLockChecker` — tracks a set of `DeadLockThreadExecutor` heartbeats (one per monitored thread, `bump()`ed periodically) and invokes an alerter callback if any goes silent past a timeout; `runner()` runs the check loop on the calling thread until `stop()`. `PerfCheck` is a small `steady_clock` stopwatch. |
| [`nstl/signal_barrier.hpp`](nstl/nstl/signal_barrier.hpp) | `nstl::SignalBarrier` — lets a thread block on `wait()` or `wait_for(timeout)` until a `SIGINT`/`SIGTERM`/`SIGQUIT` arrives. On Linux it reads from the non-blocking `signalfd` opened by `global_init` (constructed with `signal_init_=true`), blocking via `epoll`/`timerfd` rather than polling. Elsewhere it installs signal handlers (and a `SetConsoleCtrlHandler` on Windows), restoring the previous ones on destruction, and polls at a configurable period (default 10ms). Only one instance may be active at a time. |
| [`nstl/vector.hpp`](nstl/nstl/vector.hpp) | `nstl::vector<T>` — a header-only, `malloc`/`realloc`-backed vector restricted to trivially-copyable `T`, with `push_back()`/`pop_back()`/`reserve()`/`clear()`/`at()`, iterator/`data()` access, and `release()` to hand off ownership as a `(pointer, size, capacity)` tuple. |
| [`nstl/args_editor.hpp`](nstl/nstl/args_editor.hpp) | `nstl::is_arg_set()` — scans `argv` for a given flag, removing every matching entry in place (shifting `argc`/`argv` down) and returning whether it was found. |
| [`nstl/backtrace.hpp`](nstl/nstl/backtrace.hpp) | `nstl::bt::backtrace_init()` / `pr_backtrace()` — prints a symbolized stack trace to an `ostream`, optionally prefixed with a function/file/line. Backed by [libbacktrace](https://github.com/ianlancetaylor/libbacktrace) on Linux and `dbghelp` (`CaptureStackBackTrace`/`SymFromAddr`) on Windows; a no-op stub elsewhere. `backtrace_init()` is called once by `global_init`. |
| [`nstl/safe_basename.hpp`](nstl/nstl/safe_basename.hpp) | `nstl::safe_basename_view` — returns the filename portion of a path as a `constexpr string_view` (overloaded for `wstring_view` too), platform-aware (`\\` on Windows, `/` elsewhere), without allocating. |
| [`nstl/string.hpp`](nstl/nstl/string.hpp) | `nstl::split_view_func`, `nstl::trim_view` / `left_trim_view` / `right_trim_view` — header-only `string_view` helpers to split on a delimiter (invoking a callback per token) and trim leading/trailing whitespace, without allocating. |
| [`nstl/temp_dir.hpp`](nstl/nstl/temp_dir.hpp) | `nstl::temp_dir` — RAII wrapper that creates a temporary directory (random, named, or under a given parent) and removes it on destruction. |
| [`nstl/memory.hpp`](nstl/nstl/memory.hpp) | `nstl::observer_ptr` (alias `leaking_ptr`) — a non-owning smart pointer, header-only: constructible from a raw pointer, `unique_ptr` (any deleter), or `shared_ptr`, with the usual comparison operators, `make_observer()` factories, and a `std::hash` specialization. |
| [`nstl/base64.hpp`](nstl/nstl/base64.hpp) | `nstl::to_base64` / `nstl::from_base64` — header-only base64 encode/decode over iterator pairs or ranges, built on Boost.Archive's base64 iterators. |
| [`nstl/macros.hpp`](nstl/nstl/macros.hpp) | `NSTL_THROW_EXCEPTION` / `NSTL_THROW_EXCEPTION_IF` — throw an exception of a caller-chosen type, with a message prefixed by file (via `safe_basename_view`) and line number. |
| [`nstl/parse.hpp`](nstl/nstl/parse.hpp) | `nstl::parse_view<T>()` — parses an arithmetic type (or `bool`, from `"true"`/`"false"`/`"1"`/`"0"`) out of a `string_view` via `std::from_chars`, throwing if the view is empty, isn't fully consumed, or isn't a valid number. |
| [`nstl/exception.hpp`](nstl/nstl/exception.hpp) | `nstl::exception` — a `std::runtime_error` that records the throw site's file/line and prints nested exception chains via `operator<<`. The `NSTL2_THROW_EXCEPTION` / `NSTL2_THROW_EXCEPTION_IF` macros throw one with a message prefixed by file (via `safe_basename_view`) and line number; `NSTL2_NESTED_THROW_EXCEPTION` does the same via `std::throw_with_nested`, to chain onto a caught exception. |
| [`nstl/env_var_raii.hpp`](nstl/nstl/env_var_raii.hpp) | `nstl::env_var_raii` — RAII guard that sets (or clears) an environment variable and restores its previous value on destruction. `get_env_var()` reads one without throwing if unset. |
| [`nstl/logging.hpp`](nstl/nstl/logging.hpp) | `nstl::log::Logger` and the `NSTL_DEBUG` / `NSTL_INFO` / `NSTL_WARNING` / `NSTL_ERROR` / `NSTL_TERMINATE` macros — leveled, timestamped logging to a file, `ostream`, or a custom sink function, with a timezone-aware timestamp via `LogTimeZone`. Log lines (`` timestamp\|file:line\|thread-id\|LEVEL\|message ``) are handed off to a background thread through a TBB concurrent queue, so callers don't block on I/O. Constructing a `Logger` pushes it onto a stack (only one console logger may be active at a time) with its own level; the innermost one receives log calls, and the previous one — and its level — resumes when it's destroyed. `throttleSize()` caps the queue so a stalled sink can't grow unbounded; `NSTL_TERMINATE` logs, flushes, and calls `std::abort()`. |
| [`nstl/datahash.hpp`](nstl/nstl/datahash.hpp) / [`nstl/datahash_fwd.hpp`](nstl/nstl/datahash_fwd.hpp) | `nstl::Hasher` — a streaming MD5/SHA1/SHA256/SHA512 hasher (`HashType`) whose `add()` takes raw bytes, a `span` of any trivially-copyable type, a `string_view`, or a NUL-terminated `char*`/`wchar_t*`. `hash_file()` hashes a file directly (optionally into a caller-supplied buffer), and `hash_to_hex()` / `whash_to_hex()` render a readable (narrow or wide) digest. Backed by OpenSSL on Linux/macOS and Windows CryptoAPI on Windows. |
| [`nstl/global_init.hpp`](nstl/nstl/global_init.hpp) | `nstl::global_init` — RAII one-time process-wide setup/teardown; throws if constructed more than once. Runs `bt::backtrace_init()`, then `curl_global_init`/`curl_global_cleanup` when the HTTP client is enabled (otherwise Winsock `WSAStartup`/`WSACleanup` on Windows — curl initializes Winsock itself, so both aren't needed). Pass `signal_init_=true` to also block `SIGINT`/`SIGTERM`/`SIGQUIT` and open a `signalfd` on Linux (exposed via the static `getSignalFile()`) for `SignalBarrier` to read from. Construct one instance before using networking components. |
| [`nstl/http_client.hpp`](nstl/nstl/http_client.hpp) | `nstl::http::Client` — a libcurl-based HTTP client: `get()` / `post()` (each taking an optional `duration` connect/total timeout, defaulting to 10s, and following up to 10 redirects), `add_header()`, `reset_hdrs()` to clear just the accumulated headers, `url_encode()` / `url_decode()`, plus the free functions `nstl::http::is_http_success()` / `is_ssl_supported()`. `nstl::url::is_valid_url()` validates a URL per RFC 3986 and, given a `view_results` out-param, also returns the matched protocol/hostname/path/params spans (`ResIdx`). Requires `nstl::global_init` to have run first. |

## Tools

`bin/multi_runner` launches N copies of a command as parallel child processes
(`fork`/`execve` on POSIX, `CreateProcess` on Windows), each with
`NSTL_PROCESSES_NUMBER` (total count) and `NSTL_PROCESSES_ID` (0-based index)
set in its environment. Each child's stdout is piped back and forwarded to the
runner's own stdout, and `SIGINT`/`SIGTERM`/`SIGQUIT` received by the runner
are forwarded to every child. It waits for all children, and if any exits
non-zero, terminates the rest and propagates that exit code:

```sh
multi_runner <thread-count> <command> [args...]
```

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
