/*
  SRM AP Notice Board - ESP8266 (NodeMCU / Wemos D1 mini) sensor node
  Sensors : DHT11 (temperature + humidity)  -> D4
            MQ135 (air quality, analog)     -> A0
  Libraries (Library Manager): "DHT sensor library" + "Adafruit Unified Sensor"
  Board   : ESP8266 core by ESP8266 Community
  Note    : NodeMCU A0 accepts 0-3.3 V only. If your MQ135 module outputs up to 5 V,
            use a divider (e.g. 10k + 20k) before A0. Let MQ135 warm up ~2 minutes.
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <DHT.h>

// ---------- EDIT THESE ----------

const char* WIFI_SSID  = "SRM-SD ESIOT";
const char* WIFI_PASS  = "password";
const char* SB_HOST    = "bfsizsjopmmyhmsmxxpt.supabase.co";   
const char* SB_ANON    = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJmc2l6c2pvcG1teWhtc214eHB0Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3OTMyNjM5ODMsImV4cCI6MjEwODgzOTk4M30.fHyEfN7f3kHEg4Cs9vJXsJ_QiJVcDYqLceSSi3faXcw";
const char* DEVICE_KEY = "CHANGE_ME_LONG_RANDOM_DEVICE_KEY";
// --------------------------------

#define DHTPIN  D4
#define DHTTYPE DHT11
const unsigned long PUSH_EVERY = 10000;   // heartbeat interval

DHT dht(DHTPIN, DHTTYPE);
BearSSL::WiFiClientSecure tls;
BearSSL::Session session;                 // lets reconnects skip the heavy TLS handshake
HTTPClient http;                          // kept alive: one TLS connection reused for every beat
char url[96], body[200];
unsigned long lastPush = 0;

void wifiUp() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi");
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(500); Serial.print("."); }
  Serial.println(WiFi.status() == WL_CONNECTED ? " connected" : " failed");
}

void push() {
  float t = dht.readTemperature(), h = dht.readHumidity();
  long g = 0;
  for (int i = 0; i < 8; i++) { g += analogRead(A0); delay(5); }
  g /= 8;
  char ts[12] = "null", hs[12] = "null";
  if (!isnan(t)) dtostrf(t, 1, 1, ts);
  if (!isnan(h)) dtostrf(h, 1, 0, hs);
  snprintf(body, sizeof(body), "{\"device_key\":\"%s\",\"t\":%s,\"h\":%s,\"g\":%ld,\"rs\":%d,\"up\":%lu}",
           DEVICE_KEY, ts, hs, g, WiFi.RSSI(), millis() / 1000);

  int code = -1;
  if (http.begin(tls, url)) {
    http.setReuse(true);
    http.setTimeout(15000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", SB_ANON);
    http.addHeader("Authorization", String("Bearer ") + SB_ANON);
    code = http.POST((uint8_t*)body, strlen(body));
    if (code > 0 && code != 204 && code != 200) Serial.println("Server said: " + http.getString());
    http.end();
  }
  Serial.printf("T=%s H=%s GAS=%ld -> HTTP %d  (free heap %u)\n", ts, hs, g, code, ESP.getFreeHeap());
  if (code < 0) {                          // drop the broken connection, retry in 3 s
    Serial.println(http.errorToString(code));
    tls.stop();
    lastPush = millis() - PUSH_EVERY + 3000;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Reset reason: " + ESP.getResetReason());   // tells us WHY it restarted
  dht.begin();
  WiFi.setSleepMode(WIFI_NONE_SLEEP);      // steadier Wi-Fi
  WiFi.setAutoReconnect(true);
  tls.setInsecure();                       // skips certificate check (simple); pin a cert for production
  tls.setSession(&session);
  snprintf(url, sizeof(url), "https://%s/rest/v1/rpc/esp_push", SB_HOST);
  wifiUp();
}

void loop() {
  wifiUp();
  if (WiFi.status() == WL_CONNECTED && millis() - lastPush >= PUSH_EVERY) {
    lastPush = millis();
    push();
  }
  delay(50);
}
