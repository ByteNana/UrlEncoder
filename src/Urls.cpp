#include "Urls.h"

#include <esp_log.h>

std::function<std::shared_ptr<Client>(bool secure)> URLs::_clientFactory = nullptr;

void URLs::setClientFactory(std::function<std::shared_ptr<Client>(bool secure)> factory) noexcept {
  _clientFactory = std::move(factory);
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
  _protocol = extractProtocol();

  if (_protocol == "http") {
    _type = URLType::HTTP;
  } else if (_protocol == "https") {
    _type = URLType::HTTPS;
  } else {
    _type = URLType::UNKNOWN;
  }
  if (_type == URLType::UNKNOWN) {
    log_e("Missing or invalid protocol\n");
    return false;
  }

  _domain = extractDomain();
  _port = extractPort();
  if (_domain.find('.') == std::string::npos && _domain != "localhost") {
    log_e("Missing or invalid domain\n");
    return false;
  }

  _path = extractPath();
  if (_path.empty()) {
    log_e("Missing or invalid path\n");
    return false;
  }

  if (_address.find(' ') != std::string::npos) {
    log_e("URL contains whitespaces\n");
    return false;
  }

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
  if (!_clientFactory) {
    log_e("No client factory set. Call URLs::setClientFactory() first.\n");
    return nullptr;
  }
  if (_type == URLType::UNKNOWN) {
    log_e("URL not yet parsed. Call isValid() first.\n");
    return nullptr;
  }
  return _clientFactory(_type == URLType::HTTPS);
}
