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
└── _clientFactory  static  std::function<shared_ptr<Client>(bool)>
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
           missing domain dot / spaces in the address
```

`isValid()` is intentionally destructive-accumulating: it overwrites `_protocol`, `_domain`, `_path`, `_port`, and `_type` every time it is called, which makes re-validation after `setAddress()` safe.

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

The library is decoupled from any specific network stack. The application registers a factory once at startup:

```cpp
URLs::setClientFactory([](bool secure) -> std::shared_ptr<Client> {
  if (secure) return std::make_shared<WiFiClientSecure>();
  return std::make_shared<WiFiClient>();
});
```

`getClient()` calls `_clientFactory(_type == URLType::HTTPS)`, so the caller never needs to inspect the protocol — the factory receives a single `bool` that carries the security requirement. The factory is a `static` member so it is shared across all `URLs` instances in a translation unit.

`getClient()` returns `nullptr` (and logs an error) if the factory has not been set.

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
