# nstl

A lightweight, header-first C++20 extension library. `nstl` fills in small,
commonly needed gaps around the standard library — timezone conversion, DNS
lookups, an HTTP client, file hashing, structured logging, deadlock detection,
and a handful of RAII helpers (scope guards, temp dirs, env vars) — without
pulling in a heavyweight dependency.

## Requirements

- C++20 compiler (GCC, Clang, or MSVC)
- CMake 3.28+
The third-party libraries below (oneTBB, GoogleTest, Boost, libcurl) are all
resolved the same way on every platform: CMake first tries `find_package(... QUIET)`
for an installed copy, and only if that fails fetches and builds it from source
via `FetchContent` (the fetched version is a cache variable you can override).

- [oneTBB](https://github.com/uxlfoundation/oneTBB) — used for the async logging queue. Found via `find_package(TBB)` (e.g. `apt install libtbb-dev`), otherwise fetched (`NSTL_TBB_VERSION`, default `v2023.1.0`)
- [GoogleTest](https://github.com/google/googletest) — found via `find_package(GTest)` (e.g. `apt install libgtest-dev googletest`), otherwise fetched (`NSTL_GTEST_VERSION`, default `v1.18.0`)
- [Boost](https://www.boost.org/) (`headers`, `serialization`, `tokenizer` and `program_options` components) — `headers` backs `base64.hpp` (Boost.Archive's base64 iterators) and `csv_dict_reader.hpp` (Boost.Tokenizer, Boost.Container); `program_options` is used by `spf_resolver` and the tests. Found via `find_package(Boost COMPONENTS headers serialization program_options tokenizer)` (e.g. `apt install libboost-all-dev`), otherwise fetched (`NSTL_BOOST_VERSION`, default `boost-1.92.0`)
- [libcurl](https://curl.se/libcurl/) — required, used by the HTTP client. Found via `find_package(CURL COMPONENTS HTTPS SSL)` (e.g. `apt install libcurl4-openssl-dev`), otherwise downloaded and built from source as a static, Schannel-backed library (`NSTL_CURL_VERSION`, default `8.22.0`) — Schannel is Windows-only, so on Linux/macOS an installed libcurl is effectively required
- Hashing backend (`datahash.hpp`), chosen by the `NSTL_USING_OPENSSL` cache option (`ON` by default on Linux, `OFF` on Windows/macOS): [OpenSSL](https://www.openssl.org/) when `ON` (found via `find_package(OpenSSL REQUIRED)`, e.g. `apt install libssl-dev`), otherwise Windows CryptoAPI (`Advapi32.lib`) or macOS CommonCrypto
- Windows: links against `Ws2_32.lib`, `Dnsapi.lib`, `Advapi32.lib` (hashing via CryptoAPI), and `dbghelp.lib` (symbolized stack traces)
- Linux: links against `resolv` for DNS. It also links against `backtrace` (libbacktrace, typically bundled with GCC) for symbolized stack traces *if* a `try_compile` check at configure time (`NSTL_HAVE_BACKTRACE`, see `cmake/backtrace_is_around.cpp`) finds it; otherwise `backtrace.hpp` falls back to a no-op stub, same as on macOS
- macOS: links against `resolv`; hashing uses CommonCrypto (no OpenSSL needed); stack traces (`backtrace.hpp`) are a no-op stub on this platform
- Whether `nstl::c_timezone`/`nstl::log::LogTimeZone` are built on `std::chrono` directly or on the [HowardHinnant/date](https://github.com/HowardHinnant/date) library is decided at configure time by a `try_compile` check (`NSTL_USING_STD_CHRONO`, see `cmake/chrono_has_tzone.cpp`) that probes whether the standard library's `std::chrono::time_zone` support is complete. Where it isn't (currently macOS/libc++), `NSTL_USING_HH_DATE` is defined and the HH date library is built from the `modules/date` git submodule
- Git — read at configure time (`cmake/FindNstl.cmake`) via `git rev-parse --short HEAD` to stamp the build with a version string (`nstl::version()`, see Components); falls back to a `0` build component if Git or a `.git` directory isn't found

## Building

```sh
git submodule update --init --recursive   # only needed where NSTL_USING_STD_CHRONO is off (currently macOS), for the HH date library
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

## Components

| Header | Description |
| --- | --- |
| [`nstl/c_timezone.hpp`](nstl/nstl/c_timezone.hpp) | `nstl::c_timezone` — converts between local and system time using the platform C library (works around incomplete `std::chrono::time_zone` support on some standard library implementations); built on the [HowardHinnant/date](https://github.com/HowardHinnant/date) library instead of `std::chrono` directly wherever `NSTL_USING_STD_CHRONO` is off (currently macOS, see Requirements). Also exposes `nstl::time::localtime_r` / `gmtime_r` / `timegm` / `mktime` — thread-safe, cross-platform wrappers over the C time API (`*_s` on Windows, `*_r` on POSIX). |
| [`nstl/dns_tools.hpp`](nstl/nstl/dns_tools.hpp) | `nstl::net` — DNS helpers backed by native resolver APIs (Windows DNS API / POSIX `resolv`), each taking a `std::string_view` name and returning `nullopt` on failure: `hostname()`, `canonical_name()`, `ips_name()` (A/AAAA lookup, filtered by `IpClass`: `Ipv4`, `Ipv6`, or `IpvAll`), `mx_name()` (`mx_srv` records with address/priority), `txt_name()`, `c_name()`, `srv_name()` (`gen_srv` records with address/port/priority/weight), and `svcb_name()` for `SVCB`/`HTTPS` records (selected by `SvcbType`; returns `gen_svcb`, whose `params` hold a `svcb_param` variant covering mandatory keys, ALPN list, DoH path, port, and IPv4/IPv6 hint addresses). IP address types come from `ip_tools.hpp`. |
| [`nstl/ip_tools.hpp`](nstl/nstl/ip_tools.hpp) | `nstl::net` IP address/range helpers: `ipv4_addr` / `ipv6_addr` (byte arrays) and `ipv4_range` / `ipv6_range` (address + netmask, with `contains()`), plus the `ip_addr_gen` / `ip_range_gen` variants. `parse_ip4_address()` / `parse_ip6_address()` / `parse_ip_address()` and `parse_ip4_range()` / `parse_ip6_range()` (CIDR notation) convert from text; `to_string()` / `operator<<` convert back (overloaded for each address type, the variant, or a raw byte `span`). `create_ipv4_mask()` / `create_ipv6_mask()` build netmasks from a prefix length, `contains()` tests an address against a span of mixed v4/v6 ranges, `ip_range_less` is a transparent comparator for sorted containers, and `is_ipv4()` returns the embedded `ipv4_addr` if a byte span is a 4-byte address or a v4-mapped IPv6 address, or `nullopt` otherwise. |
| [`nstl/spf_tools.hpp`](nstl/nstl/spf_tools.hpp) | `nstl::net::spf` — **work in progress** SPF record parser: `parse_spf()` validates a `v=spf1 ... all` TXT record and invokes a callback with an `ip_range_gen` for every mechanism; `parse_item()` handles a single mechanism. Currently only `ip4:` / `ip6:` mechanisms are emitted; others (`a`, `mx`, `include`, ...) are ignored and a trailing `redirect=` is accepted but not followed yet. |
| [`nstl/csv_dict_reader.hpp`](nstl/nstl/csv_dict_reader.hpp) | `nstl::csv::csv_dict_reader` — a row-by-row CSV reader over an `istream`, built on Boost.Tokenizer's `escaped_list_separator`. `next()` advances a row; `header()` / `row()` expose the parsed column mapping and current values, `get_field()` looks up a value by column name, and `get_data()` returns the row as a name → value map. Behavior is configurable via `csv_parse_config` (header parsing, empty-line skipping, column-count checks, comment/escape/separator/quote characters). Free functions `parse_csv_line()`, `write_field()` and `write_row()` parse a single line and write correctly quoted/escaped CSV output. |
| [`nstl/ro_buffer.hpp`](nstl/nstl/ro_buffer.hpp) | `nstl::basic_ro_buffer` — a read-only `std::streambuf` wrapping an existing `char`/`wchar_t` buffer, `string_view`, or `span`, so you can build an `istream` over memory without copying. |
| [`nstl/range_print.hpp`](nstl/nstl/range_print.hpp) | `range_print` / `range_map_print` (and the iterator-pair forms `range_print_iter` / `range_map_print_iter`) — stream a range (or map-like range, with a separate key/value delimiter) to an `ostream` with a custom delimiter, without manually looping. The range overloads require an lvalue, since the returned object holds iterators into it. |
| [`nstl/scope_exit.hpp`](nstl/nstl/scope_exit.hpp) | `nstl::scope_exit` / `on_scope_exit` — RAII guard that runs a callable when the scope exits; `reset()` cancels it, `swap()` exchanges its callable, and `empty()` / `explicit operator bool()` report whether one is set. |
| [`nstl/unlock_guard.hpp`](nstl/nstl/unlock_guard.hpp) | `nstl::unlock_guard` — the inverse of `std::lock_guard`: unlocks one or more mutexes for the current scope and re-locks them on destruction. |
| [`nstl/dead_lock_detect.hpp`](nstl/nstl/dead_lock_detect.hpp) | `nstl::DeadLockChecker` — tracks a set of `DeadLockThreadExecutor` heartbeats (one per monitored thread, `bump()`ed periodically) and invokes an alerter callback if any goes silent past a timeout; `runner()` runs the check loop on the calling thread until `stop()`. `PerfCheck` is a small `steady_clock` stopwatch. |
| [`nstl/signal_barrier.hpp`](nstl/nstl/signal_barrier.hpp) | `nstl::SignalBarrier` — lets a thread block on `wait()` or `wait_for(timeout)` until a `SIGINT`/`SIGTERM`/`SIGQUIT` arrives. On Linux it reads from the non-blocking `signalfd` opened by `global_init` (constructed with `signal_init_=true`), blocking via `epoll`/`timerfd` rather than polling. Elsewhere it installs signal handlers (and a `SetConsoleCtrlHandler` on Windows), restoring the previous ones on destruction, and polls at a configurable period (default 10ms). Only one instance may be active at a time. |
| [`nstl/vector.hpp`](nstl/nstl/vector.hpp) | `nstl::vector<T>` — a header-only, `malloc`/`realloc`-backed vector restricted to trivially-copyable `T`, with `push_back()`/`pop_back()`/`reserve()`/`clear()`/`at()`/`erase()` (single element or iterator range), bounds-checked iterator/`data()` access, and `release()` to hand off ownership as a `(pointer, size, capacity)` tuple. |
| [`nstl/secure_string.hpp`](nstl/nstl/secure_string.hpp) | `nstl::strlen` / `strcmp` / `strncmp` / `wcslen` — null-safe wrappers over the C string functions (returning 0, or an arbitrary but consistent ordering, instead of crashing on a null pointer). `secure_zero()` zeroes a buffer via `memset`, for scrubbing sensitive data before it's freed. |
| [`nstl/args_editor.hpp`](nstl/nstl/args_editor.hpp) | `nstl::is_arg_set()` — scans `argv` for a given flag, removing every matching entry in place (shifting `argc`/`argv` down) and returning whether it was found; throws on a negative `argc`, a null `argv`/flag, or an empty flag. |
| [`nstl/backtrace.hpp`](nstl/nstl/backtrace.hpp) | `nstl::bt::backtrace_init()` / `pr_backtrace()` — prints a symbolized stack trace to an `ostream`, optionally prefixed with a function/file/line. Backed by [libbacktrace](https://github.com/ianlancetaylor/libbacktrace) on Linux (when detected, see Requirements) and `dbghelp` (`CaptureStackBackTrace`/`SymFromAddr`) on Windows; a no-op stub elsewhere. `backtrace_init()` is called once by `global_init`. |
| [`nstl/handle_raii.hpp`](nstl/nstl/handle_raii.hpp) | `nstl::custom_raii<Type, Tools>` — a generic RAII wrapper for a trivial handle type: move-only, with `reset()`, `release()`, `swap()`, an `explicit operator bool()`/`operator Type()`, and comparison operators against both another instance and a raw `Type`. `Tools` supplies `invalid_value`, `valid()`, and `free()`. Ready-made aliases: `HandleRaii` (Windows `HANDLE`, closed via `CloseHandle`) and `FileIntRaii` (POSIX file descriptor, closed via `close`). |
| [`nstl/math.hpp`](nstl/nstl/math.hpp) | `nstl::math::float_eq<nan_eq>` — a floating-point equality comparator (`operator()` / `eq()`) that special-cases infinities (compares sign), zero, and NaN (equal only if `nan_eq`), and otherwise compares within an epsilon. `safe_llround()` converts to `long long`, throwing if the value can't be represented; `round_val<TgtT>()` / `round_floor<TgtT>()` / `round_ceil<TgtT>()` round a floating value to an integral type, throwing on overflow/underflow. |
| [`nstl/compiler.hpp`](nstl/nstl/compiler.hpp) | Compiler-specific pragma macros: `NSTL_FENV_ACCESS_ON` enables floating-point environment access (used by `math.hpp`'s `safe_llround`); `NSTL_WRN_SWITCH_ENUM_PUSH`/`POP`, `NSTL_WRN_DATE_PUSH`/`POP` and `NSTL_DEPRECATED_PUSH`/`POP` push/pop warning suppressions (switch-over-enum; old-style-cast and switch warnings around the HH date headers; deprecated declarations, e.g. CommonCrypto's MD5) — no-ops on MSVC. |
| [`nstl/file_trigger.hpp`](nstl/nstl/file_trigger.hpp) | `nstl::file_trigger` — watches a file (by path or an already-open `native_handle`) for appended data on a background thread and invokes a `data_cb(std::span<char>)` callback with each chunk read; `factory()` optionally reads what's already in the file first (`sow_`), takes a read `buffer_size_` (default ~1MB), and returns a `shared_ptr<file_trigger>` whose `stop()` ends the watch. Backed by `inotify`+`epoll` on Linux, `ReadDirectoryChangesW` on Windows, and `kqueue`/`FSEvents` on macOS. |
| [`nstl/safe_basename.hpp`](nstl/nstl/safe_basename.hpp) | `nstl::safe_basename_view` — returns the filename portion of a path as a `constexpr string_view` (overloaded for `wstring_view` too), platform-aware (`\\` on Windows, `/` elsewhere), without allocating. |
| [`nstl/string.hpp`](nstl/nstl/string.hpp) | `nstl::split_view_func`, `nstl::trim_view` / `left_trim_view` / `right_trim_view` — header-only `string_view` helpers to split on a delimiter (invoking a callback per token) and trim leading/trailing whitespace, without allocating. `nstl::iequal` compares two views case-insensitively. |
| [`nstl/temp_dir.hpp`](nstl/nstl/temp_dir.hpp) | `nstl::temp_dir` — RAII wrapper that creates a temporary directory (random, named, or under a given parent) and removes it on destruction. `nstl::temp_file` is the file counterpart: creates (and removes) a temporary file opened `Write`- or `ReadWrite`-mode, movable, with `path()` and a native-handle conversion via `handle_type`/`FileIntRaii`. |
| [`nstl/memory.hpp`](nstl/nstl/memory.hpp) | `nstl::observer_ptr` (alias `leaking_ptr`) — a non-owning smart pointer, header-only: constructible from a raw pointer, `unique_ptr` (any deleter), or `shared_ptr`, with the usual comparison operators, `make_observer()` factories, and a `std::hash` specialization. |
| [`nstl/base64.hpp`](nstl/nstl/base64.hpp) | `nstl::to_base64` / `nstl::from_base64` — header-only base64 encode/decode over iterator pairs or ranges, built on Boost.Archive's base64 iterators. |
| [`nstl/macros.hpp`](nstl/nstl/macros.hpp) | `NSTL_THROW_EXCEPTION` / `NSTL_THROW_EXCEPTION_IF` — throw an exception of a caller-chosen type, with a message prefixed by file (via `safe_basename_view`) and line number. |
| [`nstl/parse.hpp`](nstl/nstl/parse.hpp) | `nstl::parse_view<T>()` — parses an arithmetic type (or `bool`, from `"true"`/`"false"`/`"1"`/`"0"`) out of a `string_view` via `std::from_chars`, throwing if the view is empty, isn't fully consumed, or isn't a valid number. |
| [`nstl/exception.hpp`](nstl/nstl/exception.hpp) | `nstl::exception` — a `std::runtime_error` that records the throw site's file/line and prints nested exception chains via `operator<<`. The `NSTL2_THROW_EXCEPTION` / `NSTL2_THROW_EXCEPTION_IF` macros throw one with a message prefixed by file (via `safe_basename_view`) and line number; `NSTL2_NESTED_THROW_EXCEPTION` does the same via `std::throw_with_nested`, to chain onto a caught exception. |
| [`nstl/env_var_raii.hpp`](nstl/nstl/env_var_raii.hpp) | `nstl::env_var_raii` — RAII guard that sets (or clears) an environment variable and restores its previous value on destruction. `get_env_var()` reads one without throwing if unset. |
| [`nstl/logging.hpp`](nstl/nstl/logging.hpp) / [`nstl/logger.hpp`](nstl/nstl/logger.hpp) | The `NSTL_DEBUG` / `NSTL_INFO` / `NSTL_WARNING` / `NSTL_ERROR` / `NSTL_TERMINATE` macros, level handling and `LogTimeZone` (`logging.hpp`), and the `nstl::log::Logger` sink owner plus the `LoggerAPI` interface for custom sinks (`logger.hpp`) — leveled, timestamped logging to a file, `ostream`, or a custom sink function, with a timezone-aware timestamp via `LogTimeZone`. Log lines (`` timestamp\|file:line\|thread-id\|LEVEL\|message ``) are handed off to a background thread through a TBB concurrent queue, so callers don't block on I/O. Constructing a `Logger` pushes it onto a stack (only one console logger may be active at a time) with its own level; the innermost one receives log calls, and the previous one — and its level — resumes when it's destroyed. `throttleSize()` caps the queue so a stalled sink can't grow unbounded; `NSTL_TERMINATE` logs, flushes, and calls `std::abort()`. |
| [`nstl/datahash.hpp`](nstl/nstl/datahash.hpp) / [`nstl/datahash_fwd.hpp`](nstl/nstl/datahash_fwd.hpp) | `nstl::Hasher` — a streaming MD5/SHA1/SHA256/SHA512 hasher (`HashType`, default SHA256; `parseHashType()` parses one from its name) created via `Hasher::factory()`, whose `add()` takes raw bytes, a `span` of any trivially-copyable type, a `string_view`, or a NUL-terminated `char*`/`wchar_t*`, and whose `finish()` returns a `HashValue` variant (`get_hash_type()` recovers its type). `hash_file()` hashes a file directly (optionally into a caller-supplied buffer), and `hash_to_hex()` / `whash_to_hex()` / `hash_to_base64()` / `whash_to_base64()` render a readable (narrow or wide) digest. Backed by OpenSSL on Linux, CommonCrypto on macOS, and Windows CryptoAPI on Windows (see `NSTL_USING_OPENSSL` in Requirements). |
| [`nstl/global_init.hpp`](nstl/nstl/global_init.hpp) | `nstl::global_init` — RAII one-time process-wide setup/teardown; throws if constructed more than once. Runs `bt::backtrace_init()`, then `curl_global_init`/`curl_global_cleanup` by default, or — if constructed with `curl_init_=false` — just Winsock (`WSAStartup`/`WSACleanup`) on Windows (curl initializes Winsock itself, so both aren't needed). Pass `signal_init_=true` to also block `SIGINT`/`SIGTERM`/`SIGQUIT` and open a `signalfd` on Linux (exposed via the static `getSignalFile()`) for `SignalBarrier` to read from. Construct one instance before using networking components. |
| [`nstl/http_client.hpp`](nstl/nstl/http_client.hpp) | `nstl::http::Client` — a movable, libcurl-based HTTP client: `get()` / `post()` (each taking an optional `duration` connect/total timeout, defaulting to 10s, following up to 10 redirects, keeping TCP keepalive on, and aborting if the transfer drops below a minimum bandwidth for too long — see `minimumBandwidth()`), `add_header()`, `reset_hdrs()` to clear just the accumulated headers, `setHdrCb()` to stream response header lines to a callback, `url_encode()` / `url_decode()`, plus the free functions `nstl::http::is_http_success()` / `is_ssl_supported()`. `nstl::url::is_valid_url()` validates a URL per RFC 3986 and, given a `view_results` out-param, also returns the matched protocol/hostname/path/params spans (`ResIdx`). Requires `nstl::global_init` to have run first. |
| [`nstl/version.hpp`](nstl/nstl/version.hpp) | `nstl::version()` — returns the library's `major.minor.build` version string (currently `0.5.<build>`), where `build` is the short Git commit hash of the checkout it was built from (or `0` if Git wasn't available at configure time; see Requirements). Generated at configure time from `version.cpp.in` and `cmake/FindNstl.cmake`. |

## Tools

`bin/multi_runner` launches N copies of a command as parallel child processes
(`fork`/`execve` on Linux/macOS, `CreateProcess` on Windows), each with
`NSTL_PROCESSES_NUMBER` (total count) and `NSTL_PROCESSES_ID` (0-based index)
set in its environment. It waits for all children, and if any exits non-zero,
terminates the rest and propagates that exit code. The three platform
implementations differ beyond that: on Linux each child's stdout is piped
back and forwarded to the runner's own stdout, and `SIGINT`/`SIGTERM`/`SIGQUIT`
received by the runner (via `epoll`+`signalfd`) are forwarded to every child;
on Windows a received `CTRL_*` console event is relayed to the children as a
console-control event, escalating to `TerminateProcess` after a 1s grace
period; on macOS (`kqueue`+`EVFILT_PROC`/`NOTE_EXIT`) only the "kill the rest
on first failure" behavior applies — the runner's own signals aren't relayed
and child stdout isn't piped.

```sh
multi_runner <thread-count> <command> [args...]
```

`bin/test/multi_test` is a small helper for exercising `multi_runner` by hand:
it logs its `NSTL_PROCESSES_ID` / `NSTL_PROCESSES_NUMBER` and arguments, waits
on a `SignalBarrier`, and exits with the received signal's value — or with
success if started with `--healthy`. It isn't installed or registered with CTest.

`spf_resolver/spf_resolver` is an SPF resolver command-line tool built on
`spf_tools.hpp`. It is **work in progress**: it currently only parses its
command line (Boost.Program_options) and does not perform any lookup yet.

```sh
spf_resolver --domain <domain>   # -d <domain>; -h / --help shows the options
```

Both `multi_runner` and `spf_resolver` are installed into `bin` by `cmake --install`.

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
#include <nstl/logger.hpp>   // nstl::log::Logger (also pulls in logging.hpp's macros)

nstl::log::Logger logger{ std::filesystem::path{"app.log"}, nstl::log::level::Info };

NSTL_INFO("Started with " << argc << " arguments");
NSTL_ERROR("Failed to open " << path);
```

## License

MIT — see [LICENSE](LICENSE).
