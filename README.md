# 🛰️ ESP32 IoT Object Counter (Firebase & Web Server)

A smart IoT solution for real-time object detection and counting using an **ESP32** microcontroller and an **HC-SR04 ultrasonic sensor**. This project features live data synchronization with **Google Firebase** and a built-in **Web Dashboard** for local monitoring.

---

## ✨ Key Features
* **Smart Detection Logic:** Uses hysteresis (`THRESHOLD` and `HYSTERESIS`) to prevent double-counting and ensure stable detection even if an object lingers.
* **Firebase Integration:** Real-time updates for total counts, distance, and a persistent history log (`detection_log`).
* **NTP Time Synchronization:** Automatically fetches accurate local time (configured for Serbia/Europe) for precise event logging.
* **Local Web Server:** Hosts a live dashboard directly on the ESP32 (accessible via its local IP address).
* **Remote Reset:** Reset the counter remotely via Firebase (by toggling `reset_flag` to `true`) or via the web interface.
* **Configurable Cooldown:** Prevents rapid accidental triggers using a programmable delay (default set to 140s).

---

## 🛠️ Hardware Requirements
1.  **ESP32** (e.g., DevKit V1)
2.  **HC-SR04** Ultrasonic Sensor
3.  Jumper Wires

### Pinout Configuration
| HC-SR04 Pin | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **VCC** | 5V / VIN | Power Supply |
| **Trig** | GPIO 5 | Trigger Pin |
| **Echo** | GPIO 18 | Echo Pin |
| **GND** | GND | Ground |



---

## 🚀 Getting Started

### 1. Firebase Setup
* Go to the [Firebase Console](https://console.firebase.google.com/).
* Create a new project and add a **Realtime Database**.
* Set Database **Rules** to `true` for read/write (for initial testing).
* Copy your Database URL (e.g., `https://your-db-default-rtdb.firebaseio.com`).

### 2. Code Configuration
Update the following constants in the `.ino` file before uploading:

#define WIFI_SSID   "Your_WiFi_Name"
#define WIFI_PASS   "Your_WiFi_Password"
#define FB          "[https://your-project-url.firebaseio.com](https://your-project-url.firebaseio.com)"

3. Required Libraries
Install these via the Arduino Library Manager:

ArduinoJson (by Benoit Blanchon)

WiFi (Built-in)

HTTPClient (Built-in)

WebServer (Built-in)

4. Installation
Connect your ESP32 to your computer.

Open the code in the Arduino IDE.

Select DOIT ESP32 DEVKIT V1 (or your specific model) and the correct Port.

Click Upload.

Open the Serial Monitor (115200 baud) to see the device's local IP address.

⚙️ Adjustable Parameters (Tuning)
You can fine-tune the sensor behavior by modifying these values in the code:

THRESHOLD (200.0f): Detection distance in cm. If an object is closer than this, it counts.

HYSTERESIS (250.0f): The object must move further than this distance before the sensor allows a new detection.

COOLDOWN (140000UL): Minimum milliseconds (e.g., 140 seconds) between two valid counts to avoid noise.

📊 Firebase Data Structure
The device maintains the following structure in your database:

{
  "distance": 120.5,
  "object_count": 15,
  "last_detection": "23.03.2026 14:20:05",
  "sensor_near": true,
  "detection_log": {
    "-Nxyz...": { "count": 15, "time": "23.03.2026 14:20:05" }
  }
}

🌐 Local Web Interface
Once connected to WiFi, you can visit the ESP32's IP address (e.g., http://192.168.100.160) to view:

Current Distance: Real-time ultrasonic readings.

Total Object Count: Total items/people detected.

Status: "DETEKTOVAN" (Detected) or "Slobodno" (Clear).

System Time: Synchronized via NTP.

