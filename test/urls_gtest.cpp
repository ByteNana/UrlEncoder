#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Urls.h"

class MockClient : public Client {
 public:
  MOCK_METHOD(int, connect, (IPAddress ip, uint16_t port), (override));
  MOCK_METHOD(int, connect, (const char *host, uint16_t port), (override));
  MOCK_METHOD(size_t, write, (uint8_t), (override));
  MOCK_METHOD(size_t, write, (const uint8_t *buf, size_t size), (override));
  MOCK_METHOD(int, available, (), (override));
  MOCK_METHOD(int, read, (), (override));
  MOCK_METHOD(int, read, (uint8_t *buf, size_t size), (override));
  MOCK_METHOD(int, peek, (), (override));
  MOCK_METHOD(void, flush, (), (override));
  MOCK_METHOD(void, stop, (), (override));
  MOCK_METHOD(uint8_t, connected, (), (override));
  operator bool() override { return true; }
};

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

TEST(UrlsConstructors, StdStringConstructor) {
  std::string addr("https://example.com/path");
  URLs url(addr);
  EXPECT_TRUE(url.isValid());
  EXPECT_STREQ(url.getDomain(), "example.com");
}

TEST(UrlsConstructors, StdStringSetAddress) {
  URLs url;
  url.setAddress(std::string("http://example.com/path"));
  EXPECT_TRUE(url.isValid());
}

// ---------------------------------------------------------------------------
// isValid
// ---------------------------------------------------------------------------

TEST(UrlsValid, HttpUrl) {
  URLs url("http://example.com/path");
  EXPECT_TRUE(url.isValid());
}

TEST(UrlsValid, HttpsUrl) {
  URLs url("https://example.com/path");
  EXPECT_TRUE(url.isValid());
}

TEST(UrlsValid, UrlWithCustomPort) {
  URLs url("http://example.com:8080/path");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getPort(), 8080);
}

TEST(UrlsValid, Localhost) {
  URLs url("http://localhost/path");
  EXPECT_TRUE(url.isValid());
}

TEST(UrlsValid, LocalhostWithPort) {
  URLs url("http://localhost:3000/path");
  EXPECT_TRUE(url.isValid());
  EXPECT_EQ(url.getPort(), 3000);
}

TEST(UrlsValid, UrlWithQueryString) {
  URLs url("https://api.example.com/v1/data?key=value&foo=bar");
  EXPECT_TRUE(url.isValid());
}

TEST(UrlsValid, MissingProtocol) {
  URLs url("example.com/path");
  EXPECT_FALSE(url.isValid());
}

TEST(UrlsValid, InvalidProtocol) {
  URLs url("ftp://example.com/path");
  EXPECT_FALSE(url.isValid());
}

TEST(UrlsValid, MissingDomainDot) {
  URLs url("http://nodot/path");
  EXPECT_FALSE(url.isValid());
}

TEST(UrlsValid, UrlWithSpaces) {
  URLs url("https://example.com/path with spaces");
  EXPECT_FALSE(url.isValid());
}

TEST(UrlsValid, EmptyAddress) {
  URLs url("");
  EXPECT_FALSE(url.isValid());
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

TEST(UrlsGetters, Protocol) {
  URLs url("https://example.com/path");
  url.isValid();
  EXPECT_STREQ(url.getProtocol(), "https");
}

TEST(UrlsGetters, Domain) {
  URLs url("https://example.com/path");
  url.isValid();
  EXPECT_STREQ(url.getDomain(), "example.com");
}

TEST(UrlsGetters, DomainStripsPort) {
  URLs url("https://example.com:8443/path");
  url.isValid();
  EXPECT_STREQ(url.getDomain(), "example.com");
}

TEST(UrlsGetters, Path) {
  URLs url("https://example.com/some/path");
  url.isValid();
  EXPECT_STREQ(url.getPath(), "/some/path");
}

TEST(UrlsGetters, DefaultHttpPort) {
  URLs url("http://example.com/path");
  url.isValid();
  EXPECT_EQ(url.getPort(), 80);
}

TEST(UrlsGetters, DefaultHttpsPort) {
  URLs url("https://example.com/path");
  url.isValid();
  EXPECT_EQ(url.getPort(), 443);
}

TEST(UrlsGetters, IsSecureHttp) {
  URLs url("http://example.com/path");
  url.isValid();
  EXPECT_FALSE(url.isSecure());
}

TEST(UrlsGetters, IsSecureHttps) {
  URLs url("https://example.com/path");
  url.isValid();
  EXPECT_TRUE(url.isSecure());
}

// ---------------------------------------------------------------------------
// encode
// ---------------------------------------------------------------------------

TEST(UrlsEncode, PreservesQueryString) {
  URLs url("https://api.example.com/v1?key=hello&foo=bar");
  EXPECT_TRUE(url.encode());
  std::string addr(url.getAddress());
  EXPECT_NE(addr.find('?'), std::string::npos);
  EXPECT_NE(addr.find('&'), std::string::npos);
  EXPECT_NE(addr.find('='), std::string::npos);
}

TEST(UrlsEncode, EncodesSpaces) {
  URLs url("https://example.com/path with spaces");
  url.encode();
  std::string addr(url.getAddress());
  EXPECT_NE(addr.find("%20"), std::string::npos);
  EXPECT_EQ(addr.find(' '), std::string::npos);
}

TEST(UrlsEncode, EncodesSpecialChars) {
  URLs url("https://example.com/path?q=hello world&name=foo@bar");
  url.encode();
  std::string addr(url.getAddress());
  EXPECT_EQ(addr.find(' '), std::string::npos);
  EXPECT_NE(addr.find("%20"), std::string::npos);
  EXPECT_NE(addr.find("%40"), std::string::npos);
}

TEST(UrlsEncode, ValidAfterCleanUrl) {
  URLs url("https://api.example.com/v1/resource");
  EXPECT_TRUE(url.encode());
}

TEST(UrlsEncode, UpdatesAddress) {
  URLs url("https://example.com/hello world");
  url.encode();
  EXPECT_STREQ(url.getAddress(), "https://example.com/hello%20world");
}

// ---------------------------------------------------------------------------
// getClient / factory
// ---------------------------------------------------------------------------

TEST(UrlsClient, ReturnsNullWithoutFactory) {
  URLs::setClientFactory(nullptr);
  URLs url("https://example.com/path");
  url.isValid();
  EXPECT_EQ(url.getClient(), nullptr);
}

TEST(UrlsClient, FactoryCalledWithSecureTrueForHttps) {
  bool calledWithSecure = false;
  URLs::setClientFactory([&](bool secure) -> std::shared_ptr<Client> {
    calledWithSecure = secure;
    return std::make_shared<MockClient>();
  });

  URLs url("https://example.com/path");
  url.isValid();
  auto client = url.getClient();

  EXPECT_TRUE(calledWithSecure);
  EXPECT_NE(client, nullptr);

  URLs::setClientFactory(nullptr);
}

TEST(UrlsClient, FactoryCalledWithSecureFalseForHttp) {
  bool calledWithSecure = true;
  URLs::setClientFactory([&](bool secure) -> std::shared_ptr<Client> {
    calledWithSecure = secure;
    return std::make_shared<MockClient>();
  });

  URLs url("http://example.com/path");
  url.isValid();
  auto client = url.getClient();

  EXPECT_FALSE(calledWithSecure);
  EXPECT_NE(client, nullptr);

  URLs::setClientFactory(nullptr);
}
