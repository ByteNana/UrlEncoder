# UrlEncoder

Arduino/ESP32 library for parsing, validating, and percent-encoding HTTP/HTTPS URLs. Designed to be dropped into IoT firmware as a reusable building block — it handles URL hygiene and hands back a ready-to-use network `Client` via a factory you define at boot.

## Features

- Parses protocol, domain, path, and port from a URL string
- Validates HTTP/HTTPS scheme, domain format, and absence of unencoded spaces
- Percent-encodes unsafe characters in place, preserving query-string delimiters (`?`, `&`, `=`, `#`)
- Returns a `std::shared_ptr<Client>` via a layered factory — a static default shared across all instances, overridable per-instance or at construction time
- C++11, no heap allocation beyond `std::string`

## Installation

### PlatformIO

Add to `platformio.ini`:

```ini
lib_deps =
  https://github.com/ByteNana/UrlEncoder
```

### CMake (host / unit tests)

```cmake
include(FetchContent)
FetchContent_Declare(UrlEncoder GIT_REPOSITORY https://github.com/ByteNana/UrlEncoder GIT_TAG main)
FetchContent_MakeAvailable(UrlEncoder)
target_link_libraries(your_target PRIVATE UrlEncoder)
```

## Quick start

```cpp
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "Urls.h"

// Call once at boot, before any URLs instance is used
URLs::setDefaultFactory([](bool secure) -> std::shared_ptr<Client> {
  if (secure) return std::make_shared<WiFiClientSecure>();
  return std::make_shared<WiFiClient>();
});

URLs url("https://api.example.com/v1/data?key=hello world");

if (!url.encode()) {
  // invalid URL — error already logged via log_e
  return;
}

Serial.printf("Encoded : %s\n", url.getAddress());  // spaces → %20
Serial.printf("Domain  : %s\n", url.getDomain());
Serial.printf("Port    : %d\n", url.getPort());      // 443 (default)
Serial.printf("Secure  : %s\n", url.isSecure() ? "yes" : "no");

auto client = url.getClient();  // shared_ptr<WiFiClientSecure>
```

## API

### Construction

```cpp
URLs url("https://example.com/path");   // const char*
URLs url(someArduinoString);            // const String&
URLs url(someStdString);                // const std::string&
URLs url(std::move(someStdString));     // std::string&&
URLs url;                               // default, use setAddress() before anything
```

### Core methods

| Method | Returns | Description |
|---|---|---|
| `isValid()` | `bool` | Parses the address and validates it. Populates all getters. |
| `encode()` | `bool` | Percent-encodes `_address` in place, then calls `isValid()`. |
| `getClient()` | `shared_ptr<Client>` | Calls the factory with `secure=true/false`. Returns `nullptr` if no factory is set or if `isValid()` has not been called yet. |

### Setters

```cpp
url.setAddress("http://other.com/path");  // replaces address; call isValid()/encode() again
```

### Getters

All getters return data populated by the last `isValid()` or `encode()` call.

```cpp
url.getProtocol()  // "http" or "https"
url.getDomain()    // "example.com" (no port)
url.getPath()      // "/path?query=string"
url.getPort()      // explicit port, or 80/443 default
url.getType()      // URLType::HTTP | URLType::HTTPS | URLType::UNKNOWN
url.isSecure()     // true for HTTPS
url.getAddress()   // current address string (encoded after encode())
```

### Client factory

The factory resolves in priority order: **instance factory → default factory**.

```cpp
// Global default — shared across all URLs instances (set once at boot)
URLs::setDefaultFactory([](bool secure) -> std::shared_ptr<Client> {
  if (secure) return std::make_shared<WiFiClientSecure>();
  return std::make_shared<WiFiClient>();
});

// Per-instance override — set after construction or injected at construction
url.setInstanceFactory([](bool secure) -> std::shared_ptr<Client> { ... });

// Constructor injection — instance factory set at construction time
URLs url("https://example.com/path", [](bool secure) -> std::shared_ptr<Client> { ... });
```

Pass `nullptr` to `setInstanceFactory()` to clear the per-instance override and fall back to the default. Pass `nullptr` to `setDefaultFactory()` to remove the global factory entirely — no fallback exists in that case.

## Testing with mocks

The factory pattern makes `URLs` testable without real network clients. In a GoogleTest `SetUp()`, use `setDefaultFactory()` to inject a mock:

```cpp
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "Urls.h"

class MockClient : public Client {
 public:
  MOCK_METHOD(int, connect, (IPAddress ip, uint16_t port), (override));
  MOCK_METHOD(int, connect, (const char *host, uint16_t port), (override));
  // ... remaining Client methods
};

TEST(MyFeature, SendsRequestToCorrectUrl) {
  auto mock = std::make_shared<MockClient>();
  URLs::setMockClient(mock);  // convenience wrapper — same mock returned for HTTP and HTTPS

  URLs url("https://api.example.com/v1/update");
  url.isValid();
  auto client = url.getClient();  // returns mock

  // use client with your HTTP logic...

  URLs::setDefaultFactory(nullptr);  // clean up after the test
}
```

`setMockClient(mock)` is a shorthand for the common case where you want one fixed client regardless of the `secure` flag. For finer control (e.g. asserting a `WiFiClientSecure` is used for HTTPS), use `setDefaultFactory()` directly:

```cpp
URLs::setDefaultFactory([&](bool secure) -> std::shared_ptr<Client> {
  EXPECT_TRUE(secure);  // assert HTTPS is required
  return std::make_shared<MockClient>();
});
```

Both helpers affect the global default factory, so always reset it to `nullptr` after each test to avoid state leaking between cases.

## Development

### Prerequisites

```sh
make install-deps   # clang-format, bump-my-version, convco, lefthook
```

Requires: CMake ≥ 3.15, a C++11 compiler, Rust (for convco), PlatformIO (for examples).

### Workflow

```sh
make build          # configure + compile (generates compile_commands.json)
make test           # build + run GoogleTest suite
make examples       # cross-compile ESP32 example via pio ci
make check          # clang-format dry-run
make lint           # clang-tidy (requires make build first)
```

### Versioning

Version is declared in `CMakeLists.txt` and `library.json`. Use `bump-my-version` to update both atomically:

```sh
make bump           # patch bump (default): 1.0.0 → 1.0.1
make bump PART=minor
make bump PART=major
```

This creates a commit with message `chore: bump version to X.Y.Z`. The `auto-tag` CI workflow creates a GitHub release when the commit lands on `master`.

### Commit style

Commits must follow [Conventional Commits](https://www.conventionalcommits.org/) — enforced by `convco` on `commit-msg`. Direct pushes to `master` are blocked; use a PR with squash-merge.

## Architecture

See [docs/architecture.md](docs/architecture.md).

## License

MIT
