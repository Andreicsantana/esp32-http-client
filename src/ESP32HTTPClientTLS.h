#ifndef ESP32_HTTP_CLIENT_TLS_H
#define ESP32_HTTP_CLIENT_TLS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <new>

#ifndef ESP32HTTPCLIENT_REQUIRE_VALIDATED_TLS
#define ESP32HTTPCLIENT_REQUIRE_VALIDATED_TLS 0
#endif

#ifndef HTTPC_ERROR_TLS_NOT_VALIDATED
#define HTTPC_ERROR_TLS_NOT_VALIDATED (-12)
#endif

#if defined(ESP32HTTPCLIENT_LOG_W) && defined(ESP32HTTPCLIENT_LOG_E)
// Custom log macros already defined
#elif defined(log_w) && defined(log_e)
#define ESP32HTTPCLIENT_LOG_W(fmt, ...) log_w(fmt, ##__VA_ARGS__)
#define ESP32HTTPCLIENT_LOG_E(fmt, ...) log_e(fmt, ##__VA_ARGS__)
#else
#define ESP32HTTPCLIENT_LOG_W(fmt, ...) do { (void)(fmt); } while (0)
#define ESP32HTTPCLIENT_LOG_E(fmt, ...) do { (void)(fmt); } while (0)
#endif

namespace esp32_http_client_detail {

static constexpr time_t kMinValidTlsEpoch = 1672531200;  // 2023-01-01 00:00:00 UTC

inline time_t currentEpoch() {
#if defined(ESP32HTTPCLIENT_CUSTOM_EPOCH_FN)
  return ESP32HTTPCLIENT_CUSTOM_EPOCH_FN();
#else
  return time(nullptr);
#endif
}

}  // namespace esp32_http_client_detail

#if defined(HTTPCLIENT_NOSECURE)
#define ESP32HTTPCLIENT_HAS_TLS 0
#elif defined(__has_include)
#if __has_include(<NetworkClientSecure.h>)
#include <NetworkClientSecure.h>
#define ESP32HTTPCLIENT_HAS_TLS 1
#define ESP32HTTPCLIENT_TLS_NETWORK_CLIENT 1
#elif __has_include(<WiFiClientSecure.h>)
#include <WiFiClientSecure.h>
#define ESP32HTTPCLIENT_HAS_TLS 1
#define ESP32HTTPCLIENT_TLS_NETWORK_CLIENT 0
#endif
#endif

#ifndef ESP32HTTPCLIENT_HAS_TLS
#define ESP32HTTPCLIENT_HAS_TLS 0
#endif

#if ESP32HTTPCLIENT_HAS_TLS

namespace esp32_http_client_detail {

#if ESP32HTTPCLIENT_TLS_NETWORK_CLIENT
typedef NetworkClientSecure SecureClient;
#else
typedef WiFiClientSecure SecureClient;
#endif

template <typename T>
auto applyCACertBundle(T& client, const uint8_t* bundle, size_t size, int)
    -> decltype(client.setCACertBundle(bundle, size), bool()) {
  client.setCACertBundle(bundle, size);
  return true;
}

template <typename T>
auto applyCACertBundle(T& client, const uint8_t* bundle, size_t, long)
    -> decltype(client.setCACertBundle(bundle), bool()) {
  client.setCACertBundle(bundle);
  return true;
}

template <typename T>
bool applyCACertBundle(T&, const uint8_t*, size_t, ...) {
  return false;
}

class SecureClientSlot {
 public:
  SecureClientSlot() : _client(nullptr) {
  }

  ~SecureClientSlot() {
    destroy();
  }

  SecureClient* get() const {
    return _client;
  }

  SecureClient& create() {
    if (!_client) {
      _client = new (_storage) SecureClient();
    }
    return *_client;
  }

  void destroy() {
    if (_client) {
      _client->~SecureClient();
      _client = nullptr;
    }
  }

 private:
  SecureClientSlot(const SecureClientSlot&);
  SecureClientSlot& operator=(const SecureClientSlot&);

  alignas(SecureClient) unsigned char _storage[sizeof(SecureClient)];
  SecureClient* _client;
};

}

#endif

#endif
