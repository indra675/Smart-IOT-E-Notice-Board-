<div align="center">

# 📟 Smart IoT E-Notice Board

### Live room air-quality monitoring + a digital notice board, on one ESP8266 and one web page

![ESP8266](https://img.shields.io/badge/MCU-ESP8266%20NodeMCU-blue?logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Firmware-Arduino%20C%2B%2B-00979D?logo=arduino&logoColor=white)
![Supabase](https://img.shields.io/badge/Backend-Supabase-3ECF8E?logo=supabase&logoColor=white)
![HTML](https://img.shields.io/badge/Frontend-HTML%20%7C%20CSS%20%7C%20JS-E34F26?logo=html5&logoColor=white)
![Status](https://img.shields.io/badge/Status-Working%20Prototype-success)

</div>

---

## 💡 What is this?

Most campuses never measure the air inside their rooms, and notices are still printed and pinned to boards. This project fixes both with cheap parts and free cloud tools.

A **NodeMCU ESP8266** reads temperature, humidity and air quality every 10 seconds and sends them to **Supabase** over HTTPS. A web dashboard turns the raw numbers into things people understand (AQI, CO₂, feels-like, dew point) and draws a 3-hour trend. Below the live data sits an **E-Notice Board** where an admin can post notices with images, videos or YouTube links.

## ✨ Highlights

- 🌡️ **Live readings** of temperature, humidity and air quality, with online/offline status and last-sync time
- 📊 **Smart dashboard** with estimated AQI, CO₂, feels-like, dew point, absolute humidity, harmful gas index and a per-gas breakdown
- 📈 **3-hour chart** with Temperature / CO₂ / AQI tabs
- 🎨 **Animated themes:** sunrise, autumn, rain, snow, plus light/dark mode; works on phones and big display walls
- 📢 **E-Notice Board** with 7 categories, media support and admin login (publish, edit, remove)
- 🔐 **Secure by design:** HTTPS upload, device-key check, and notice access rules enforced inside the database
- ♻️ **Self-recovering firmware:** Wi-Fi auto-reconnect, TLS session reuse, 3-second retry on failure

## 🧭 How it works

```
 ┌──────────────┐    HTTPS / JSON     ┌──────────────┐     reads     ┌──────────────────┐
 │  NodeMCU     │  every 10 seconds   │   Supabase   │ ────────────► │  Web Dashboard   │
 │  + DHT11     │ ──────────────────► │  PostgreSQL  │               │  + E-Notice Board│
 │  + MQ135     │   (device key)      │  esp_push()  │ ◄──────────── │  (admin login)   │
 └──────────────┘                     └──────────────┘    notices    └──────────────────┘
      SENSING                           NETWORK + CLOUD                  APPLICATION
```

1. The board joins Wi-Fi and opens a secure connection.
2. Every 10 s it reads the DHT11 and averages 8 MQ135 samples.
3. It sends a small JSON message to the protected `esp_push` endpoint.
4. The dashboard shows the board as online and calculates the extra values and chart.
5. An admin logs in to publish, edit or remove notices.

## 🔧 Hardware

| Part | Role |
|---|---|
| NodeMCU ESP8266 (ESP-12E) | Controller with built-in Wi-Fi |
| DHT11 | Temperature and humidity |
| MQ135 | Analog air-quality sensor |
| Breadboard + jumper wires + micro-USB | Wiring and power |

**Wiring**

| Sensor | Pin | NodeMCU |
|---|---|---|
| DHT11 | DATA | `D4` |
| MQ135 | AO | `A0` |
| Both | VCC / GND | 3V3 or 5V (per module) / GND |

> [!WARNING]
> NodeMCU's `A0` accepts **0 to 3.3 V only**. If your MQ135 module outputs up to 5 V, add a voltage divider (for example 10 kΩ + 20 kΩ) before `A0`, or the board can be damaged.

## 🚀 Getting started

**1. Wire it up** as shown above.

**2. Install** the Arduino IDE, the *ESP8266 board package* (select **NodeMCU 1.0**) and two libraries: `DHT sensor library` and `Adafruit Unified Sensor`.

**3. Prepare Supabase:** create a project with the readings table, the notices table and the `esp_push` function, and register your device key.

**4. Edit the firmware** in `dashboard.ino`:

```cpp
const char* WIFI_SSID  = "your-wifi-name";
const char* WIFI_PASS  = "your-wifi-password";
const char* SB_HOST    = "your-project.supabase.co";
const char* SB_ANON    = "your-supabase-anon-key";
const char* DEVICE_KEY = "a-long-random-device-key";
```

**5. Upload** and open the Serial Monitor at `115200` baud. You should see `HTTP 204` after every reading.

**6. Wait ~2 minutes** for the MQ135 to warm up (preferably in clean air) before trusting its values.

**7. Open the web app**, check the board shows as online, then log in as admin and publish a test notice.

> [!CAUTION]
> Never commit your real Wi-Fi password or device key. Keep placeholders in the public repo.

## 📦 What the node sends

```json
{ "device_key": "...", "t": 29.4, "h": 82, "g": 470, "rs": -61, "up": 3600 }
```

| Field | Meaning |
|---|---|
| `t` / `h` | Temperature (°C) / relative humidity (%) |
| `g` | Averaged raw MQ135 value (0 to 1023) |
| `rs` | Wi-Fi signal strength (dBm) |
| `up` | Uptime in seconds |

## ✅ Test results

| Test | Result |
|---|:---:|
| Serial monitor upload shows `HTTP 204` every cycle | ✅ |
| Dashboard shows board online, last sync a few seconds old | ✅ |
| Live reading 29.4 °C / 82 % RH with matching 3-hour trend | ✅ |
| Free heap stable at about 20 KB across repeated sends | ✅ |
| All themes, carousel and chart tabs | ✅ |
| Notices published, shown, edited and removed | ✅ |

<details>
<summary><b>🛠️ Troubleshooting</b></summary>

<br>

| Problem | Likely cause | Try this |
|---|---|---|
| Wi-Fi failed | Wrong credentials or weak signal | Re-check name/password, move closer |
| Negative HTTP code | Broken TLS connection or no internet | Firmware retries in 3 s; check network |
| HTTP 401 / 403 | Wrong anon key or unregistered device key | Check both keys and `esp_push` rules |
| Temp/humidity empty | DHT11 wiring or timing | Check `D4` and power |
| Gas stuck near 0 or 1023 | MQ135 not warmed up or miswired | Wait 2 min; check `A0` and divider |
| Board shows offline | No heartbeat for over 60 s | Check power, Wi-Fi, reset reason |
| Keeps restarting | Weak USB power or low memory | Use a good adapter; watch free heap |

</details>

## ⚠️ Limitations

- DHT11 is only ±2 °C / ±5 % RH accurate
- Gas values are **estimates** from a single MQ135 analog pin, so treat them as indicative, not lab grade
- Server certificate is not verified (`setInsecure()`); pin a certificate for real deployments
- One sensor node supported for now

## 🔮 Future scope

- [ ] Better sensors (DHT22 / BME280) and dust (PM2.5 / PM10) sensing
- [ ] Multiple nodes and rooms with a location selector
- [ ] Email / SMS / push alerts on unsafe readings
- [ ] Data export and periodic reports
- [ ] OTA firmware updates, battery or solar power
- [ ] Mobile notifications for new notices
- [ ] Enclosure and custom PCB

---

## 🙋 Author

**Galla Indranag**

B.Tech ECE
Embedded Systems & IoT Analyst (ESSCI-certified), (SRM AP)

- GitHub: [@indra675](https://github.com/indra675)
- LinkedIn: [linkedin.com/in/gallaindranag](https://linkedin.com/in/gallaindranag)
