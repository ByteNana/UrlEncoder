#include "Urls.h"

#include <esp_log.h>

std::function<std::shared_ptr<Client>(bool secure)> URLs::_defaultFactory = nullptr;

void URLs::setDefaultFactory(std::function<std::shared_ptr<Client>(bool secure)> factory) noexcept {
  _defaultFactory = std::move(factory);
}

std::string URLs::extractProtocol() {
  size_t protocolEnd = _address.find("://");
  if (protocolEnd != std::string::npos) { return _address.substr(0, protocolEnd); }
  return "";
}

std::string URLs::extractDomain() {
  size_t protocolEnd = _address.find("://");
  if (protocolEnd == std::string::npos) { return ""; }
  size_t domainStart = protocolEnd + 3;
  size_t domainEnd = _address.find('/', domainStart);
  if (domainEnd == std::string::npos) { domainEnd = _address.length(); }

  std::string domain = _address.substr(domainStart, domainEnd - domainStart);

  size_t colonPos = domain.find(':');
  if (colonPos != std::string::npos) { return domain.substr(0, colonPos); }

  return domain;
}

std::string URLs::extractPath() {
  size_t protocolEnd = _address.find("://");
  if (protocolEnd == std::string::npos) { return ""; }
  size_t domainStart = protocolEnd + 3;
  size_t domainEnd = _address.find('/', domainStart);
  if (domainEnd == std::string::npos) { return "/"; }
  return _address.substr(domainEnd);
}

int URLs::extractPort() {
  size_t protocolEnd = _address.find("://");
  if (protocolEnd == std::string::npos) { return -1; }

  size_t domainStart = protocolEnd + 3;
  size_t domainEnd = _address.find('/', domainStart);
  if (domainEnd == std::string::npos) { domainEnd = _address.length(); }

  std::string hostPort = _address.substr(domainStart, domainEnd - domainStart);
  size_t colonPos = hostPort.find(':');
  if (colonPos == std::string::npos) { return -1; }

  std::string portStr = hostPort.substr(colonPos + 1);
  for (char c : portStr) {
    if (isdigit(c) == 0) {
      log_e("Invalid port (non-digit characters)\n");
      return -1;
    }
  }

  int port = atoi(portStr.c_str());
  if (port <= 0 || port > 65535) {
    log_e("Port out of valid range\n");
    return -1;
  }
  return port;
}

bool URLs::isValid() {
  resetParsed();

  const std::string proto = extractProtocol();
  URLType type = URLType::UNKNOWN;
  if (proto == "http") {
    type = URLType::HTTP;
  } else if (proto == "https") {
    type = URLType::HTTPS;
  }
  if (type == URLType::UNKNOWN) {
    log_e("Missing or invalid protocol\n");
    return false;
  }

  const std::string domain = extractDomain();
  if (domain.find('.') == std::string::npos && domain != "localhost") {
    log_e("Missing or invalid domain\n");
    return false;
  }

  const std::string path = extractPath();
  if (path.empty()) {
    log_e("Missing or invalid path\n");
    return false;
  }

  if (_address.find(' ') != std::string::npos) {
    log_e("URL contains whitespaces\n");
    return false;
  }

  const int port = extractPort();
  // extractPort() returns -1 for both "no port specified" and "invalid port".
  // Distinguish them by checking whether the host segment contains a colon.
  if (port == -1) {
    const size_t protoEnd = _address.find("://");
    const size_t hostStart = protoEnd + 3;
    const size_t hostEnd = _address.find('/', hostStart);
    const std::string hostSeg = _address.substr(
        hostStart, hostEnd == std::string::npos ? std::string::npos : hostEnd - hostStart);
    if (hostSeg.find(':') != std::string::npos) {
      log_e("Invalid or out-of-range port\n");
      return false;
    }
  }

  _protocol = proto;
  _type = type;
  _domain = domain;
  _port = port;
  _path = path;
  return true;
}

bool URLs::encode() {
  const char *hex = "0123456789abcdef";
  std::string encoded;
  const char *msg = _address.c_str();
  while (*msg != '\0') {
    if ((isalnum(*msg) != 0) || *msg == '-' || *msg == '_' || *msg == '.' || *msg == '~' ||
        *msg == '/' || *msg == ':' || *msg == '?' || *msg == '&' || *msg == '=' || *msg == '#') {
      encoded += *msg;
    } else {
      encoded += '%';
      encoded += hex[(unsigned char)*msg >> 4];
      encoded += hex[(unsigned char)*msg & 15];
    }
    msg++;
  }
  _address = encoded;
  return isValid();
}

std::shared_ptr<Client> URLs::getClient() {
  if (_type == URLType::UNKNOWN) {
    log_e("URL not yet parsed. Call isValid() first.\n");
    return nullptr;
  }
  auto &factory = _instanceFactory ? _instanceFactory : _defaultFactory;
  if (!factory) {
    log_e(
        "No client factory set. Call URLs::setDefaultFactory() or url.setClientFactory() first.\n");
    return nullptr;
  }
  return factory(isSecure());
}
