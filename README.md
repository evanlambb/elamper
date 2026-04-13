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