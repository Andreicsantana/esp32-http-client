---
title: Advanced ESP32 HTTP Client Features - Auth, Callbacks & Retries
description: Advanced HTTP client patterns on ESP32: Bearer JWT & Basic Auth, request callbacks, custom headers, Keep-Alive, timeout retries, and dynamic URLs.
keywords: ESP32 HTTP authentication, ESP32 Bearer token, HTTP headers ESP32, Keep-Alive ESP32, request callbacks ESP32
tags:
  - guide
  - advanced
---
# Advanced Usage

## Authentication Helpers

`ESP32HTTPClient` provides dedicated helper methods for common authentication schemes. Like `setHeader()`, these helpers configure persistent headers sent with **every subsequent request**.

### Bearer Token (JWT / OAuth)

```cpp
client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");
client.get("/api/profile").getBody("username", username, sizeof(username));
```

### HTTP Basic Auth

Pass your username and password; the library automatically formats and Base64-encodes the credentials:

```cpp
client.basic("admin", "secret123");
client.get("/admin/metrics").getBody("uptime", &uptime);
```

### API Key

Pass the header name and your API key:

```cpp
client.apiKey("x-api-key", "my-secret-api-key");
client.get("/v1/sensors").getBody("temperature", &temp);
```

---

## Custom HTTP Headers

Use `setHeader()` to register any arbitrary persistent header that will be sent with **every subsequent request** from that client instance:

```cpp
// Custom headers
client.setHeader("X-Device-ID", "ESP32-001");
client.setHeader("X-Custom-Header", "custom-value");

// Custom headers are sent on all subsequent requests
client.get("/protected/resource").getBody("data", &myVar);
```

!!! note "Header persistence"
    Headers registered with `setHeader()`, `bearer()`, `basic()`, or `apiKey()` persist for the lifetime of the client object. They are sent on every request. To change a header, call the method again with the new value.

!!! tip "Reading response headers"
    `setHeader()` sets headers to send with the request. To read or collect headers returned by the server in the response (such as `token`, `Content-Length`, `Date`, etc.), use the `.getHeader()` method on `RestRequest`. See how to collect them in the [Reading Response Headers](responses.md#reading-response-headers-with-getheader) guide or the [RestRequest::getHeader](../api/restrequest.md#extracting-response-headers) API reference.

---

## Custom Content-Type

By default, the `Content-Type` header is set to `application/json` for all requests with a body. Override it with `setContentType()`:

```cpp
client.setContentType("application/x-www-form-urlencoded");
client.post("/form").body("field", "value");
```

---

## Keep-Alive Connection Management

By default, `ESP32HTTPClient` enables HTTP **Keep-Alive**, reusing the underlying TCP/TLS connection across requests. This is the primary reason the library is **~12x faster** than the standard approach, since expensive TLS handshakes only happen once.

### When to call `end()`

After a burst of requests, if your sketch enters a long idle period or you want to free the TLS memory buffers (~45KB for an active connection), call `end()`:

```cpp
// Make several requests
client.get("/data1").getBody("val", &v1);
client.get("/data2").getBody("val", &v2);
client.get("/data3").getBody("val", &v3);

// Done for now — free the TLS memory
client.end();

delay(60000); // sleep for 60 seconds

// The next request will re-establish the connection automatically
client.get("/data4").getBody("val", &v4);
```

!!! tip "You don't need to call `end()` between requests"
    Keep-Alive is automatic. Only call `end()` when you explicitly want to free the connection memory after a period of inactivity.

---

## Changing Server URL at Runtime

You can change the target host URL or port on the fly without recreating the `ESP32HTTPClient` instance:

```cpp
ESP32HTTPClient client("https://api.v1.example.com");

// Switch to v2 endpoint or a local staging server
client.setBaseUrl("https://api.v2.example.com", 443);

// Or change port independently
client.setUrl("http://192.168.1.100");
client.setPort(8080);
```

---

## Timeout Configuration

By default, the client uses a timeout of **60000 ms** (1 minute / 60 seconds). You can configure the global default or override it on individual requests:

### Global Timeout

```cpp
// Set default timeout to 10 seconds for all future requests
client.setTimeout(10000);
```

### Per-Request Timeout

```cpp
// Fast ping with 1.5s timeout
client.get("/quick-ping")
      .timeout(1500)
      .getBody("ok", &isOk);
```

---

## Retries & Network Recovery

`ESP32HTTPClient` automatically handles network drops and stale Keep-Alive connections by retrying failed attempts up to a configured threshold. The default is `1` retry.

### Global Max Retry

```cpp
// Allow up to 3 retry attempts on connection failures
client.setMaxRetry(3);
```

### Per-Request Retry

```cpp
// Disable retry for non-idempotent operation
client.post("/payment/charge")
      .retry(0)
      .body("amount", 100);
```

---

## Callbacks (`onSuccess`, `onError`, `onResponse`)

Callbacks allow you to attach clean, asynchronous-style handlers to your requests or at the client level.

### Request-Level Callbacks

```cpp
client.get("/sensors/temp")
      .onSuccess([](int code) {
          Serial.printf("Success: HTTP %d\n", code);
      })
      .onError([](int code, const char* message) {
          Serial.printf("Request failed (%d): %s\n", code, message);
      })
      .onResponse([](int code) {
          Serial.printf("Completed with code %d\n", code);
      })
      .getBody("temperature", &temp);
```

### Client-Level Callbacks

Client-level callbacks are triggered on every request executed by that client instance:

```cpp
client.onError([](int code, const char* message) {
    Serial.printf("[Global Error Handler] Code %d: %s\n", code, message);
});
```

---

## Error Handling & Inspection

You can inspect the result of any request using the built-in helper methods:

```cpp
client.get("/users/1").getBody("name", name, sizeof(name));

if (client.isSuccess()) {
    Serial.println("User loaded successfully");
} else if (client.hasError()) {
    int code = client.getStatusCode();
    String errorMsg = client.getErrorMessage();
    Serial.printf("Failed with code %d: %s\n", code, errorMsg.c_str());
}
```

### Error Code Reference Table

#### Network / Client Errors (`code < 0`)

| Code | Constant | Description |
| :--- | :--- | :--- |
| `-1` | `HTTPC_ERROR_CONNECTION_REFUSED` | Target host refused the connection. |
| `-2` | `HTTPC_ERROR_SEND_HEADER_FAILED` | Failed to write HTTP headers to the socket. |
| `-3` | `HTTPC_ERROR_SEND_PAYLOAD_FAILED` | Failed to send request body payload. |
| `-4` | `HTTPC_ERROR_NOT_CONNECTED` | Not connected to network or socket. |
| `-5` | `HTTPC_ERROR_CONNECTION_LOST` | TCP connection terminated unexpectedly. |
| `-6` | `HTTPC_ERROR_NO_STREAM` | No response stream available. |
| `-7` | `HTTPC_ERROR_NO_HTTP_SERVER` | Server did not respond with valid HTTP. |
| `-8` | `HTTPC_ERROR_TOO_LESS_RAM` | Insufficient free RAM on ESP32. |
| `-9` | `HTTPC_ERROR_ENCODING` | Transfer encoding error. |
| `-10` | `HTTPC_ERROR_STREAM_WRITE` | Stream write failed. |
| `-11` | `HTTPC_ERROR_READ_TIMEOUT` | Timed out waiting for response data from server. |
| `-12` | `HTTPC_ERROR_TLS_NOT_VALIDATED` | HTTPS request refused by fail-closed mode (`requireValidatedTls`) because no Root CA or CA bundle is configured. |

#### HTTP Status Codes (`code > 0`)

| Code | Status | Description |
| :--- | :--- | :--- |
| `200` | OK | Request succeeded normally. |
| `201` | Created | Resource created successfully. |
| `202` | Accepted | Request accepted for processing. |
| `204` | No Content | Success, server returned empty response. |
| `400` | Bad Request | Invalid request parameters or payload. |
| `401` | Unauthorized | Missing or invalid authentication credentials. |
| `403` | Forbidden | Insufficient permissions for resource. |
| `404` | Not Found | Requested endpoint path does not exist. |
| `408` | Request Timeout | Server timed out waiting for request. |
| `429` | Too Many Requests | Rate limit exceeded. |
| `500` | Internal Server Error | Generic server-side error. |
| `502` | Bad Gateway | Invalid response from upstream server. |
| `503` | Service Unavailable | Server overloaded or down for maintenance. |
| `504` | Gateway Timeout | Upstream gateway timed out. |

---

## Long Unix Timestamps

For Unix timestamps and other large integer values (greater than `2^31 - 1`), use a `long` binding:

```cpp
long unixTimestamp = 0;

client.get("/api/v1/time/current/unix")
      .getBody("unix_timestamp", &unixTimestamp);

Serial.printf("Unix time: %ld\n", unixTimestamp);
```

---

## Struct <-> JSON Mapping

`ESP32-HTTP-Client` provides direct bidirectional mapping between C++ `struct`s and JSON payloads with zero dynamic document allocations, reflection overhead, or external dependencies.

### Declaring Mappable Structs

Use the `REST_JSON_MAP` macro inside your struct to define its JSON fields:

```cpp
struct User {
    int id = 0;
    char name[32] = {0};
    float score = 0.0f;
    bool active = false;
    String email = "";

    REST_JSON_MAP(
        REST_FIELD(id),
        REST_FIELD(name),
        REST_FIELD(score),
        REST_FIELD(active),
        REST_FIELD(email)
    )
};
```

If your JSON keys differ from the C++ member names, use `REST_FIELD_NAMED`:

```cpp
struct Profile {
    int userId = 0;
    char fullName[32] = {0};

    REST_JSON_MAP(
        REST_FIELD_NAMED("user_id", userId),
        REST_FIELD_NAMED("full_name", fullName)
    )
};
```

For structs defined in external or 3rd-party libraries, use `REST_JSON_MAP_EXT`:

```cpp
struct ExternalDevice {
    int deviceId;
    String status;
};

REST_JSON_MAP_EXT(ExternalDevice,
    REST_FIELD_EXT(deviceId),
    REST_FIELD_EXT(status)
)
```

### Sending Structs in Request Body

Pass the struct directly into `.body()`:

```cpp
User user;
user.id = 15;
strncpy(user.name, "Pedro", sizeof(user.name));
user.score = 9.8f;
user.active = true;
user.email = "pedro@example.com";

// Automatically serialized to JSON: {"id":15,"name":"Pedro","score":9.8,"active":true,"email":"pedro@example.com"}
client.post("/users").body(user);
```

### Receiving Structs from Response Body

Populate your struct directly from the HTTP response stream:

```cpp
User user;
// Binds and populates struct directly from the root JSON response
client.get("/users/15").getBody(&user);

Serial.printf("User: ID=%d, Name=%s, Active=%d\n", user.id, user.name, user.active);
```

You can also bind to nested objects within the response JSON:

```cpp
User nestedUser;
// Binds fields to "data.user.id", "data.user.name", etc.
client.get("/profile").getBody("data.user", &nestedUser);
```

### Missing and Null Fields Handling

- If the JSON response is **missing** fields that exist in the struct, those struct members keep their default values.
- If the JSON contains **extra** fields that are not in the struct, they are safely ignored.
- If a JSON field has a `null` value, primitive numbers are reset to `0`, booleans to `false`, char buffers to `""`, and Strings to `""`.

### Standalone Serialization & Deserialization (`RestJson` / `toJson` / `fromJson`)

You can also convert structs to and from JSON strings independently of HTTP requests:

```cpp
User user;
user.id = 42;
strncpy(user.name, "Ana", sizeof(user.name));

// Struct -> JSON String
String jsonStr = ESP32HTTPClient::toJson(user);
// Or: String jsonStr = RestJson::toJson(user);

// JSON String -> Struct
User parsedUser;
ESP32HTTPClient::fromJson(jsonStr, &parsedUser);
// Or: RestJson::fromJson(jsonStr, &parsedUser);
```

---

## TLS / HTTPS Security & Certificates

`ESP32HTTPClient` includes built-in support for encrypted HTTPS connections with X.509 certificate validation, client certificate authentication (mTLS), certificate bundles, fail-closed enforcement, and actionable mbedTLS error diagnostics.

### How to Obtain the Right Root CA (`openssl s_client`)

Do **not** pin the server's leaf certificate in your firmware — leaf certificates expire every 60 to 90 days (e.g., Let's Encrypt), which would break your devices as soon as the server renews its certificate. Instead, always pin the **Root CA** (the trust anchor at the top of the certificate chain).

Run `openssl s_client` on your workstation to inspect the full certificate chain presented by your target server:

```bash
openssl s_client -showcerts -servername api.example.com -connect api.example.com:443 </dev/null
```

Look at the **last certificate** in the output (or check its `i:` / `Issuer` field). Copy that Root CA PEM block (`-----BEGIN CERTIFICATE----- ... -----END CERTIFICATE-----`), or download the corresponding Root CA PEM directly from the Certificate Authority's official repository (such as **ISRG Root X1** for Let's Encrypt, **DigiCert Global Root G2**, or **Amazon Root CA 1**).

### When to Use a Single Root CA vs. a Certificate Bundle

| Approach | API | Flash Cost | When to Use |
| :--- | :--- | :--- | :--- |
| **Single Root CA** | `client.setCACert(root_ca_pem)` | **~1.5 – 2 KB** per PEM | **Recommended for single-backend IoT devices.** Use when your device talks to a fixed backend, cloud API, or MQTT/HTTPS broker whose PKI you control or know in advance. |
| **Certificate Bundle** | `client.setCACertBundle(bundle, size)` | **~25 – 65 KB** | **Recommended for multi-domain / user-configured URLs.** Use when connecting to arbitrary webhooks, public APIs, or CDNs (e.g., Cloudflare, AWS CloudFront) that may dynamically switch between different Certificate Authorities (Let's Encrypt, Google Trust Services, DigiCert). |

### Certificate & Root CA Expiration (Firmware Updates / OTA)

- **Leaf Certificates**: Renewed frequently by servers (60–90 days). Because the ESP32 validates the chain against the **Root CA**, routine leaf certificate renewals require **no firmware update**.
- **Root CA Certificates**: Root CAs have long lifetimes (typically 5–20 years), but they **do eventually expire**, can be revoked, or a cloud provider may migrate to a brand-new Root CA hierarchy (for example, the transition from DST Root CA X3 to ISRG Root X1, or Baltimore CyberTrust Root to DigiCert Global Root G2).
- **Production Strategy**: Never deploy HTTPS-connected devices without an **OTA (Over-The-Air) firmware update** mechanism—or store your trusted Root CA PEMs in an updatable partition (such as LittleFS or NVS)—so you can roll out updated Root CAs before old roots expire.

### Buffer Lifetime (Pointer Semantics)

Certificates and private keys are stored by **pointer** within `ESP32HTTPClient`. No dynamic copies are made on the heap. Therefore, ensure your certificate buffers remain valid in memory for the duration of the client:

```cpp
// Stored as static global memory or in PROGMEM
static const char* isrg_root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFazCCA1OgAwIBAgIRAIIQz7urnIjMvluidIT5FsAwDQYJKoZIhvcNAQELBQAw\n"
    "...\n"
    "-----END CERTIFICATE-----\n";

void setup() {
    client.setCACert(isrg_root_ca);
}
```

### Fail-Closed Mode (`requireValidatedTls`) & Security Warnings

By default, if an `https://` request is made without calling `setCACert()` or `setCACertBundle()`, or when `setInsecure()` is used, `ESP32HTTPClient` emits an ESP32 warning log (`ESP32HTTPCLIENT_LOG_W` / `log_w`) alerting that TLS validation is not configured or has been bypassed.

To enforce **fail-closed security** in production (refusing to connect over HTTPS if someone forgets to configure a Root CA or CA bundle, or accidentally leaves `setInsecure()` enabled), enable `requireValidatedTls(true)`:

```cpp
client.requireValidatedTls(true);
client.setCACert(isrg_root_ca); // If omitted, HTTPS requests fail immediately with -12
```

You can also enforce fail-closed mode globally across all `ESP32HTTPClient` instances at compile time in `platformio.ini` without modifying sketch code:

```ini
build_flags =
    -DESP32HTTPCLIENT_REQUIRE_VALIDATED_TLS=1
```

When fail-closed mode blocks an unvalidated HTTPS request, no network socket is opened and the request immediately returns `HTTPC_ERROR_TLS_NOT_VALIDATED` (`-12`).

### Mandatory NTP Time Synchronization

TLS certificate chains include `Not Before` and `Not After` timestamp validations. Because the ESP32 hardware lacks a battery-backed RTC, its clock defaults to Unix epoch 0 (January 1, 1970) upon boot. Without time synchronization, the TLS stack will reject certificates as not yet valid (`MBEDTLS_ERR_X509_CERT_VERIFY_FAILED`, `-0x2700`).

Always synchronize system time after WiFi connection:

```cpp
configTime(0, 0, "pool.ntp.org", "time.nist.gov");
time_t now = time(nullptr);
while (now < 1672531200) { // Jan 1, 2023
    delay(500);
    now = time(nullptr);
}
```

### Actionable TLS Error Diagnostics (`getTlsErrorHint`)

When a TLS handshake fails, a generic `"Connection Refused"` (`-1`) is rarely enough to pinpoint the root cause. `ESP32HTTPClient` automatically enriches `client.getErrorMessage()` with the underlying mbedTLS hex code and an actionable diagnostic hint, and also provides `client.getTlsErrorHint()` (and static `ESP32HTTPClient::tlsErrorToHint(tlsError, currentEpoch)`):

| Root Cause | mbedTLS Code(s) | Diagnostic Hint Returned |
| :--- | :--- | :--- |
| **System clock not synced (NTP)** | `-0x2700` (with system clock `< 2023`) | System clock is before 2023 (unsynchronized); call `configTime(0, 0, "pool.ntp.org")` via NTP before HTTPS requests. |
| **Wrong Root CA / Expired Cert / Hostname mismatch** | `-0x2700` (`X509_CERT_VERIFY_FAILED`) | Certificate verification failed: check that the Root CA matches the server chain (`openssl s_client -showcerts`), the certificate is not expired, and the URL hostname matches the certificate CN/SAN. |
| **Invalid PEM / DER format** | `-0x2180` .. `-0x3E80` | Invalid certificate/key format: verify the PEM string includes `-----BEGIN/END CERTIFICATE-----` markers, newline `\n` characters, and remains valid in memory. |
| **Low Heap Memory** | `-0x7F00`, `-0x2880`, `-0x0010` | Out of heap memory during TLS handshake (~45 KB contiguous free RAM required). Call `client.end()` on idle clients. |
| **Fatal Alert / mTLS Required** | `-0x7780` (`SSL_FATAL_ALERT_MESSAGE`) | Server rejected the TLS handshake (check if mutual TLS `setClientCert()` is required, SNI hostname, or cipher compatibility). |
| **SNI / Server Certificate Missing** | `-0x7280`, `-0x7880`, `-0x7100`, `-0x7680` | Handshake negotiation failed or no certificate matched the requested hostname (SNI). |

```cpp
client.get("/todos/1");
if (client.hasError()) {
    // Prints full error + mbedTLS hex code + actionable hint
    Serial.printf("Error (%d): %s\n",
                  client.getStatusCode(), client.getErrorMessage().c_str());

    if (client.getTlsError() != 0) {
        Serial.printf("TLS Code: -0x%04X | Hint: %s\n",
                      -client.getTlsError(), client.getTlsErrorHint().c_str());
    }
}
```

### Insecure Mode (Development Only)

For local development or testing with self-signed certificates, certificate validation can be explicitly bypassed (unless `requireValidatedTls(true)` is enabled):

```cpp
client.setInsecure();
```

!!! warning "Security Hazard"
    `setInsecure()` logs a warning and completely disables server certificate and hostname verification, making connections vulnerable to Man-in-the-Middle (MITM) attacks. Never deploy code with `setInsecure()` into production.


