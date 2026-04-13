# elamper# 💡 Synchronized IoT Friendship Lamps

A full-stack Internet of Things (IoT) project featuring a low-latency Python backend and C++ firmware. This system connects pairs of physical lamps across different Wi-Fi networks in real-time, allowing users to trigger color changes on remote devices instantly via capacitive touch.

The architecture relies on secure WebSockets for instantaneous, bi-directional communication, bypassing traditional REST polling limits to achieve near-zero latency. It features a room-based routing system, allowing multiple independent pairs of lamps to operate securely on a single deployed server instance.

---

## 🛠️ Tech Stack

**Hardware:**
* **Microcontroller:** ESP32 (NodeMCU / WROOM)
* **Actuator:** WS2812B Addressable LED Ring
* **Sensor:** TTP223 Capacitive Touch Sensor
* **Enclosure:** Custom 3D-printed housing

**Software (Firmware):**
* **Language:** C++ (Arduino Framework)
* **Libraries:** `ArduinoWebsockets` (by Gil Maimon), `ArduinoJson`, `FastLED` / `Adafruit_NeoPixel`

**Software (Backend):**
* **Framework:** FastAPI (Python 3)
* **Server:** Uvicorn (ASGI)
* **Protocol:** Secure WebSockets (`wss://`)
* **Deployment:** Render (PaaS)

---

## 📁 Repository Structure

```text
├── /server                  # FastAPI server logic
│   ├── main.py              # WebSocket routing and Connection Manager
│   └── requirements.txt     # Python dependencies
├── /hello_world             # ESP32 C++ firmware project
│   └── hello_world.ino      # Wi-Fi connection, WebSocket handling, and LED logic
└── README.md
```

## ☁️ Server Deployment (Render)
This backend is designed to be hosted for free on Render.

1. Deploying the Service
   Create a Web Service on Render.

   Connect this repository and set the Root Directory to `/server` (or deploy from the root if you prefer).

   Configure the build:
   * Runtime: Python 3
   * Build Command: `pip install -r requirements.txt`
   * Start Command: `uvicorn main:app --host 0.0.0.0 --port 10000`

   Select the Free instance type and deploy.

2. The Keep-Alive Protocol
   Render's free tier automatically spins down services after 15 minutes of inactivity. To ensure the hardware maintains an instantaneous connection, this server exposes a `/health` endpoint.

   Use `cron-job.org` to prevent the server from sleeping:
   * Target URL: `https://<your-render-url>.onrender.com/health`
   * Method: `GET`
   * Schedule: Every 14 minutes

## 🔌 Hardware Setup
1. Flashing the Firmware
   Open `/hello_world/hello_world.ino` in your Arduino IDE or PlatformIO. You must update three specific variables before compiling and flashing the ESP32:

   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";

   // Hardcode the room ID for each unique pair of lamps
   const String room_id = "pair-alpha";

   // Your deployed Render URL
   const String ws_url = "wss://<your-render-url>.onrender.com/ws/lamps/" + room_id;
   ```

2. Wiring Reference
   * TTP223 Touch Sensor: Connect SIG to ESP32 Pin 4 (or defined `TOUCH_PIN`).
   * WS2812B Data In (DI): Connect to a PWM-capable pin on the ESP32.
   * Power: Ensure both the LED ring and ESP32 share a common ground, and power the LEDs from a 5V source capable of handling the current draw.

## 📡 Network Protocol & Payload
The system uses standard JSON payloads to transmit state changes across the WebSocket connection.

Expected Payload:
When the touch sensor is triggered, the ESP32 broadcasts the following JSON to the server, which instantly routes it to all other clients in the same `room_id`:

```json
{
  "color": "#00FF00",
  "brightness": 255
}
```

## 💻 Local Development
To run the WebSocket server locally for testing:

Navigate to the backend directory:

```bash
cd server
```

Install dependencies:

```bash
pip install -r requirements.txt
```

Start the server:

```bash
uvicorn main:app --reload --host 0.0.0.0 --port 8000
```

The WebSocket endpoint will be available at: `ws://localhost:8000/ws/lamps/{room_id}`
