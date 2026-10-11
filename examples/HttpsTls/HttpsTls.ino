#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "ESP32HTTPClient.h"

// ============================================================================
// WiFi Configuration
// ============================================================================
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// ============================================================================
// TLS / HTTPS Root Certificate
//
// Root CA for jsonplaceholder.typicode.com (ISRG Root X1 / Let's Encrypt).
// IMPORTANT: Certificates and keys are stored by POINTER, not copied.
// They must remain valid in memory (e.g. static, const char*, or PROGMEM)
// for the entire lifetime of the ESP32HTTPClient instance.
// ============================================================================
static const char* isrg_root_x1_ca =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIFazCCA1OgAwIBAgIRAIIQz7urnIjMvluidIT5FsAwDQYJKoZIhvcNAQELBQAw\n"
  "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
  "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
  "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
  "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
  "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3UEtIiEGbd63QHWAgo\n"
  "blss4mdTstdeDT62k45DQjiDtg++IZgyeqGQVVlMvConJyWDJZVgq35Jg+UTprBQ\n"
  "27EPY5wMmZsX9TQtxKnJJmhNdVYfbcmkpqvBatOO764IrqJK+4biZ24bBC2NpdgK\n"
  "dgWmBVSBbdP0ZOTxN8yLWaSXZKeng76a00R3P151ZqRf8DumIiHAhNXNa2ehhPY1\n"
  "lfVhGAJKUiAP/5MU5CiSQ979hAe5MKERguCd10sj/DoYW75stB44i0AgEStTpQU3\n"
  "tpNV+WBQMVLB8g62blPx5FvV8ZfIuJ24754okutBQJFNgQIiF7557n+T8Bl/PFnw\n"
  "HY5/SW6geUR24Ja124pdQK+VolCTTr88ZW7440b+/ap5qWO3/4AcUrQQ52mTu29g\n"
  "nnXY5QLtmv3838hs5rnOWztwKA30be93i/W7Shv53239YKM7wuHP+ggWhWgNdNAg\n"
  "utNlY978p4NjPp32vvquK63EFYOwUnQ275ReuvKqNLMWZciiWwMXBSXG92/5GzBU\n"
  "1CZXFsfZhaAh Panda6d16ujgwSpqKHdd22jWLS3G3HbeDTvS92QZLrSNYPZDT65DIR50\n"
  "ht5S1383i566G6Aw646Gg2/460eXm4pvVauMtW8vAvC57N666G5g==\n"
  "-----END CERTIFICATE-----\n";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

// ============================================================================
// NTP Time Synchronization Helper
//
// TLS certificate validation relies on valid system time to verify the
// certificate's "Not Before" and "Not After" validity window.
// If the ESP32 clock is at Unix epoch 0 (1970), certificate validation WILL FAIL.
// ============================================================================
void syncNtpTime() {
  Serial.print("Synchronizing system time via NTP");
  // Set UTC offset (0 for UTC, or specify your timezone offset in seconds)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  time_t now = time(nullptr);
  // Wait until time is updated (year > 2020 -> epoch > 1577836800)
  while (now < 1577836800) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime synchronized successfully!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("Current UTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void setup() {
  Serial.begin(115200);

  // 1. Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // 2. Synchronize NTP clock (MANDATORY for TLS certificate validation)
  syncNtpTime();
}

void loop() {
  Serial.println("\n========================================================");
  Serial.printf("Initial Free Heap: %u bytes\n", ESP.getFreeHeap());

  // --------------------------------------------------------------------------
  // Example 1: Validated HTTPS Request with Root CA
  // --------------------------------------------------------------------------
  Serial.println("\n--- [1] HTTPS GET with Root CA Validation ---");

  // Configure Root CA (stored by pointer, buffer must outlive the client)
  client.setCACert(isrg_root_x1_ca);

  // Measure timing and memory for first TLS handshake
  uint32_t tStart = millis();
  uint32_t heapBefore = ESP.getFreeHeap();

  int todoId = 0;
  char title[64] = {0};

  client.get("/todos/1")
        .getBody("id", &todoId)
        .getBody("title", title, sizeof(title));

  uint32_t tDuration = millis() - tStart;
  uint32_t heapAfter = ESP.getFreeHeap();

  if (client.isSuccess()) {
    Serial.printf("Handshake & Request completed in %u ms\n", tDuration);
    Serial.printf("Heap delta (TLS buffers in memory): %d bytes\n", (int)(heapBefore - heapAfter));
    Serial.printf("Response -> Todo #%d: %s\n", todoId, title);
  } else {
    Serial.printf("Request failed! HTTP status: %d, Error: %s\n",
                  client.getStatusCode(), client.getErrorMessage().c_str());
    if (client.getTlsError() != 0) {
      Serial.printf("TLS Error Code: -0x%04X, Details: %s\n",
                    -client.getTlsError(), client.getTlsErrorMessage().c_str());
    }
  }

  // --------------------------------------------------------------------------
  // Example 2: TLS Session Reuse (HTTP Keep-Alive)
  // Subsequent requests to the same host reuse the existing TLS session,
  // avoiding a new full TLS handshake (much lower latency).
  // --------------------------------------------------------------------------
  Serial.println("\n--- [2] Subsequent HTTPS GET (Reusing TLS Session) ---");
  tStart = millis();

  client.get("/todos/2")
        .getBody("id", &todoId)
        .getBody("title", title, sizeof(title));

  tDuration = millis() - tStart;
  if (client.isSuccess()) {
    Serial.printf("Reused session request completed in %u ms (much faster!)\n", tDuration);
    Serial.printf("Response -> Todo #%d: %s\n", todoId, title);
  }

  // --------------------------------------------------------------------------
  // Example 3: Closing Connection and Freeing Memory
  // Calling end() closes the TLS socket and frees mbedTLS buffers (~45 KB).
  // --------------------------------------------------------------------------
  Serial.println("\n--- [3] Closing Connection (Freeing TLS buffers) ---");
  client.end();
  Serial.printf("Free Heap after client.end(): %u bytes\n", ESP.getFreeHeap());

  // --------------------------------------------------------------------------
  // Example 4: Insecure HTTPS (Bypass Certificate Validation)
  // WARNING: Disables server certificate and hostname verification!
  // Leaves connection vulnerable to Man-in-the-Middle (MITM) attacks.
  // Use ONLY for development, self-signed certificates, or testing.
  // --------------------------------------------------------------------------
  Serial.println("\n--- [4] Insecure HTTPS Mode (Development / Testing) ---");
  client.setInsecure();

  client.get("/todos/3")
        .getBody("id", &todoId)
        .getBody("title", title, sizeof(title));

  if (client.isSuccess()) {
    Serial.printf("Insecure request success -> Todo #%d: %s\n", todoId, title);
  }

  client.end();

  // Wait 30 seconds before next run
  delay(30000);
}
