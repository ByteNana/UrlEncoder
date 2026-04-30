# UrlEncoder

Arduino/ESP32 library for parsing, validating, and percent-encoding HTTP/HTTPS URLs. Designed to be dropped into IoT firmware as a reusable building block — it handles URL hygiene and hands back a ready-to-use network `Client` via a factory you define at boot.

## Features

- Parses protocol, domain, path, and port from a URL string
- Validates HTTP/HTTPS scheme, domain format, and absence of unencoded spaces
- Percent-encodes unsafe characters in place, preserving query-string delimiters (`?`, `&`, `=`, `#`)
- Returns a `std::shared_ptr<Client>` via a static factory — decoupled from any specific network stack
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
URLs::setClientFactory([](bool secure) -> std::shared_ptr<Client> {
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
URLs url;                               // default, use setAddress() before anything
```

### Core methods

| Method | Returns | Description |
|---|---|---|
| `isValid()` | `bool` | Parses the address and validates it. Populates all getters. |
| `encode()` | `bool` | Percent-encodes `_address` in place, then calls `isValid()`. |
| `getClient()` | `shared_ptr<Client>` | Calls the factory with `secure=true/false`. Returns `nullptr` if no factory is set. |

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

```cpp
URLs::setClientFactory([](bool secure) -> std::shared_ptr<Client> {
  ...
});
```

Set once. All `URLs` instances share it. Pass `nullptr` to clear.

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
