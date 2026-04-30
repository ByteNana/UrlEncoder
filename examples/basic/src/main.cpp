#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include "Urls.h"

void setup() {
  Serial.begin(115200);

  URLs::setDefaultFactory([](bool secure) -> std::shared_ptr<Client> {
    if (secure) { return std::make_shared<WiFiClientSecure>(); }
    return std::make_shared<WiFiClient>();
  });

  URLs url("https://api.example.com/v1/data?key=hello world&foo=bar");

  if (!url.encode()) {
    Serial.println("Invalid URL");
    return;
  }

  Serial.printf("Address  : %s\n", url.getAddress());
  Serial.printf("Protocol : %s\n", url.getProtocol());
  Serial.printf("Domain   : %s\n", url.getDomain());
  Serial.printf("Path     : %s\n", url.getPath());
  Serial.printf("Port     : %d\n", url.getPort());
  Serial.printf("Secure   : %s\n", url.isSecure() ? "yes" : "no");

  auto client = url.getClient();
  if (client) { Serial.println("Client ready"); }
}

void loop() {}
