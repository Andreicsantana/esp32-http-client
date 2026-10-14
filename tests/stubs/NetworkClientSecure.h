#ifndef NETWORK_CLIENT_SECURE_H
#define NETWORK_CLIENT_SECURE_H

#include "Arduino.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace NetworkClientSecureStub {
inline const char* lastCACert = nullptr;
inline const uint8_t* lastBundle = nullptr;
inline size_t lastBundleSize = 0;
inline const char* lastCert = nullptr;
inline const char* lastKey = nullptr;
inline bool isInsecure = false;
inline int stopCallCount = 0;
inline int lastErrorCode = 0;
inline std::string lastErrorMessage = "";

inline void reset() {
  lastCACert = nullptr;
  lastBundle = nullptr;
  lastBundleSize = 0;
  lastCert = nullptr;
  lastKey = nullptr;
  isInsecure = false;
  stopCallCount = 0;
  lastErrorCode = 0;
  lastErrorMessage.clear();
}
}  // namespace NetworkClientSecureStub

class NetworkClientSecure {
 public:
  NetworkClientSecure() {}
  virtual ~NetworkClientSecure() {}

  void stop() {
    NetworkClientSecureStub::stopCallCount++;
  }

  void setInsecure() {
    NetworkClientSecureStub::isInsecure = true;
    NetworkClientSecureStub::lastCACert = nullptr;
    NetworkClientSecureStub::lastBundle = nullptr;
    NetworkClientSecureStub::lastBundleSize = 0;
  }

  void setCACert(const char* rootCA) {
    NetworkClientSecureStub::lastCACert = rootCA;
    NetworkClientSecureStub::isInsecure = false;
  }

  void setCACertBundle(const uint8_t* bundle, size_t size = 0) {
    NetworkClientSecureStub::lastBundle = bundle;
    NetworkClientSecureStub::lastBundleSize = size;
    NetworkClientSecureStub::isInsecure = false;
  }

  void setCertificate(const char* client_ca) {
    NetworkClientSecureStub::lastCert = client_ca;
  }

  void setPrivateKey(const char* private_key) {
    NetworkClientSecureStub::lastKey = private_key;
  }

  int lastError(char* buf, const size_t size) {
    if (buf && size > 0) {
      if (!NetworkClientSecureStub::lastErrorMessage.empty()) {
        size_t len = NetworkClientSecureStub::lastErrorMessage.copy(buf, size - 1);
        buf[len] = '\0';
      } else {
        buf[0] = '\0';
      }
    }
    return NetworkClientSecureStub::lastErrorCode;
  }
};

#endif  // NETWORK_CLIENT_SECURE_H
