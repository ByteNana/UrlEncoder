/*
 * Urls.h
 *
 * Notes: Be mindful about the 'const char*' returning methods since the object
 * must still be in scope when using it!
 */
#pragma once

#include <Arduino.h>
#include <Client.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

enum class URLType : uint8_t { HTTP, HTTPS, UNKNOWN };

class URLs {
  std::string _address;
  std::string _protocol;
  std::string _domain;
  std::string _path;
  URLType _type = URLType::UNKNOWN;
  int _port = -1;

  static std::function<std::shared_ptr<Client>(bool secure)> _clientFactory;

  std::string extractProtocol();
  std::string extractDomain();
  std::string extractPath();
  int extractPort();

  void resetParsed() {
    _protocol.clear();
    _domain.clear();
    _path.clear();
    _port = -1;
    _type = URLType::UNKNOWN;
  };

 public:
  URLs(const char *address) : _address(address) { resetParsed(); };
  URLs(const String &address) : _address(address.c_str()) { resetParsed(); };
  URLs(const std::string &address) : _address(address) { resetParsed(); };
  URLs(std::string &&address) : _address(std::move(address)) { resetParsed(); };
  URLs() = default;

  static void setClientFactory(
      std::function<std::shared_ptr<Client>(bool secure)> factory) noexcept;

  bool isValid();
  bool encode();
  void setAddress(const std::string &address) {
    _address = address;
    resetParsed();
  };
  void setAddress(std::string &&address) {
    _address = std::move(address);
    resetParsed();
  };
  void setAddress(const char *address) {
    _address = address;
    resetParsed();
  };
  void setAddress(const String &address) {
    _address = address.c_str();
    resetParsed();
  };
  const char *getProtocol() { return _protocol.c_str(); };
  const char *getDomain() { return _domain.c_str(); };
  const char *getAddress() { return _address.c_str(); };
  const char *getPath() { return _path.c_str(); };
  URLType getType() { return _type; };
  bool isSecure() { return _type == URLType::HTTPS; };
  int getPort() {
    if (_type == URLType::UNKNOWN) return -1;
    if (_port != -1) return _port;
    return _type == URLType::HTTP ? 80 : 443;
  };
  std::shared_ptr<Client> getClient();
};
