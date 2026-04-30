# Architecture

## Overview

`URLs` is a single-class library that parses, validates, and percent-encodes HTTP/HTTPS URLs on Arduino/ESP32. It is designed to be used as a building block in IoT firmware: it owns the URL string and returns a ready-to-use network `Client` via a factory set once at boot.

## Class structure

```
URLs
├── _address   std::string   raw (or encoded) URL
├── _protocol  std::string   "http" | "https"
├── _domain    std::string   host without port
├── _path      std::string   path + query string
├── _port      int           explicit port, or -1
├── _type      URLType       HTTP | HTTPS | UNKNOWN
├── _defaultFactory   static  std::function<shared_ptr<Client>(bool)>
└── _instanceFactory          std::function<shared_ptr<Client>(bool)>
```

All fields start empty / sentinel (`_type = UNKNOWN`, `_port = -1`). They are populated only when `isValid()` or `encode()` is called — never in the constructor.

## Parsing pipeline

```
_address
   │
   ▼
extractProtocol()  ──►  finds "://" separator
extractDomain()    ──►  strips protocol + port from host segment
extractPath()      ──►  everything from the first "/" after the host
extractPort()      ──►  reads "host:port" segment, validates 1-65535
   │
   ▼
isValid()  assembles parsed fields, rejects unknown protocol /
           missing domain dot / spaces in the address /
           explicitly specified but out-of-range port
```

`isValid()` uses a validate-then-commit pattern: it calls `resetParsed()` first, then validates each component into local variables, and only assigns to member fields (`_protocol`, `_domain`, `_path`, `_port`, `_type`) when all checks pass. A failed intermediate check leaves all fields at their reset defaults — no partial state.

## encode()

`encode()` percent-encodes `_address` in place, then calls `isValid()` on the result and returns its return value. Characters that are passed through unchanged:

```
A-Z a-z 0-9  -  _  .  ~  /  :  ?  &  =  #
```

Everything else is encoded as `%XX` using lowercase hex digits. The unsigned-char cast before the shift prevents sign-extension on platforms where `char` is signed:

```cpp
encoded += hex[(unsigned char)*msg >> 4];
encoded += hex[(unsigned char)*msg & 15];
```

Because `encode()` calls `isValid()` internally, the parsed fields (`_domain`, `_path`, etc.) are always in sync with `_address` after a successful encode.

## Client factory pattern

The library is decoupled from any specific network stack. Factory resolution follows a two-level priority chain: **instance factory → default factory**.

### Default factory (static, shared)

Registered once at boot and shared across all `URLs` instances:

```cpp
URLs::setDefaultFactory([](bool secure) -> std::shared_ptr<Client> {
  if (secure) return std::make_shared<WiFiClientSecure>();
  return std::make_shared<WiFiClient>();
});
```

### Instance factory (per-object)

Overrides the default for a specific instance. Can be set after construction or injected at construction time:

```cpp
// post-construction
url.setClientFactory([](bool secure) -> std::shared_ptr<Client> { ... });

// constructor injection
URLs url("https://example.com/path", [](bool secure) -> std::shared_ptr<Client> { ... });
```

Passing `nullptr` to `setClientFactory()` clears the instance override and falls back to the default.

### Resolution in `getClient()`

`getClient()` returns `nullptr` (and logs an error) if `isValid()` has not been called yet (`_type == UNKNOWN`). Otherwise it picks the active factory — instance if set, default otherwise — and calls it with `isSecure()` as the `bool` argument, so the caller never needs to inspect the protocol directly.

## Design decisions

**Destructive encode** — `encode()` overwrites `_address`. There is no use case for preserving the un-encoded URL once a `URLs` object has been constructed; keeping a second copy would waste RAM on a constrained device.

**`_port = -1` sentinel** — `getPort()` returns the protocol default (80/443) when `_port` is -1. This avoids storing a redundant default and lets callers tell the difference between "no port in URL" and "port 80 explicitly in URL" if they need to.

**`URLType::UNKNOWN` sentinel** — Using a named sentinel instead of a cast integer (`(URLType)-1`) keeps the enum well-defined under C++11's `uint8_t` base type and avoids UB.

**No stdlib-only header** — `Urls.h` includes `<Arduino.h>` and `<Client.h>` because `String` and `Client` are part of the public API. Unit tests supply these via [ByteNana/ArduinoMock](https://github.com/ByteNana/ArduinoMock), which provides native stubs.

## Testing approach

Tests run on the host (no device required) using GoogleTest + ArduinoMock via CMake FetchContent. ArduinoMock's own test suite is suppressed by saving and restoring `BUILD_TESTING` around its `FetchContent_MakeAvailable` call.

`MockClient` implements all `Client` pure virtuals with `MOCK_METHOD` except `operator bool`, which GMock cannot mock — it uses a concrete `override` instead.

Test groups:

| Suite | What it covers |
|---|---|
| `UrlsValid` | `isValid()` — valid URLs, localhost, query strings, rejection cases |
| `UrlsGetters` | parsed field values after `isValid()` |
| `UrlsEncode` | space/special-char encoding, query-string preservation, address update |
| `UrlsClient` | factory null guard, `secure=true` for HTTPS, `secure=false` for HTTP |
| `UrlsClientLayered` | instance overrides default, constructor injection, fallback on clear, `secure` flag through instance path, isolation between objects |
| `UrlsSetAddress` | stale-state cleared on `setAddress()`, `-1` port and `nullptr` client before `isValid()` |
