# Synchronized IoT Friendship Lamps

A pair of ESP32-powered lamps that mirror each other in real-time over the internet. Touch one lamp and the other responds instantly — toggle power with a long press, cycle colors with a short press.

---

## How It Works

Two lamps share a **room ID**. They connect to a central **FastAPI WebSocket server** which stores the current state and relays every change. When lamp A changes, lamp B follows, and vice versa.

**Touch controls:**

| Gesture | Action |
|---|---|
| Long press (> 1 s) | Toggle power on / off |
| Short press (< 1 s) | Cycle through colors (only when on) |

**Colors:** red → green → blue → red …

Turning off always resets the color to red. Turning on starts at red.

---

## Tech Stack

**Hardware:**
* ESP32 (NodeMCU / WROOM)
* WS2812B Addressable LED Ring (12 LEDs)
* TTP223 Capacitive Touch Sensor
* Custom 3D-printed housing

**Firmware (C++ / Arduino):**
* `FastLED`, `ArduinoWebsockets` (Gil Maimon), `ArduinoJson`

**Backend (Python):**
* FastAPI + Uvicorn, deployed on Render

---

## Repository Structure

```text
├── server/
│   ├── main.py              # WebSocket relay + room state
│   └── requirements.txt     # Python dependencies
├── firmware/
│   └── firmware.ino          # Networked lamp firmware (WiFi + WebSocket)
├── hello_world/
│   └── hello_world.ino      # Basic touch-to-LED test sketch
└── README.md
```

---

## Network Protocol

Every state change is a JSON message sent over a WebSocket:

```json
{
  "power": true,
  "color": "green"
}
```

`color` is one of `"red"`, `"green"`, `"blue"`.

When a lamp connects, the server immediately sends the current room state so it syncs up.

---

## Server Deployment (Render)

1. Create a **Web Service** on Render pointing at this repo.
2. Set the **Root Directory** to `/server`.
3. Configure:
   * **Runtime:** Python 3
   * **Build Command:** `pip install -r requirements.txt`
   * **Start Command:** `uvicorn main:app --host 0.0.0.0 --port 10000`
4. Select the **Free** instance type and deploy.

### Keep-Alive

Render's free tier spins down after 15 min of inactivity. Use [cron-job.org](https://cron-job.org) to hit the health endpoint every 14 minutes:

```
GET https://<your-render-url>.onrender.com/health
```

---

## Flashing the Firmware

Open `firmware/firmware.ino` in the Arduino IDE and update the config block at the top:

```cpp
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* WS_HOST       = "YOUR_SERVER.onrender.com";
const char* ROOM_ID       = "pair-alpha";   // same value on both lamps
```

### Required Libraries (install via Arduino Library Manager)

* **FastLED**
* **ArduinoWebsockets** by Gil Maimon
* **ArduinoJson** by Benoit Blanchon

### Wiring

| Component | ESP32 Pin |
|---|---|
| WS2812B Data In | GPIO 18 |
| TTP223 Signal | GPIO 4 |
| TTP223 VCC | GPIO 15 (power hack) |

Ensure the LED ring and ESP32 share a common ground.

---

## Local Development

```bash
cd server
pip install -r requirements.txt
uvicorn main:app --reload --host 0.0.0.0 --port 8000
```

WebSocket endpoint: `ws://localhost:8000/ws/lamps/{room_id}`

For local testing with the ESP32, set `WS_HOST` to your machine's LAN IP and change the firmware to use `ws://` instead of `wss://`.
