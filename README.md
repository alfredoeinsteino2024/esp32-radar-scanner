

Readme · MD
# 🛰️ ESP32 Radar Scanner
 
**A search-and-track ultrasonic radar system, modeled on real fire-control radar behavior — built on an ESP32, streamed live to a web dashboard.**
 
![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Language](https://img.shields.io/badge/firmware-C%2B%2B-00599C)
![Backend](https://img.shields.io/badge/backend-Node.js-339933)
![Status](https://img.shields.io/badge/status-active-brightgreen)
![License](https://img.shields.io/badge/license-MIT-green)
 
---
 
## 🎯 Overview
 
Most ultrasonic "radar" projects just sweep a sensor back and forth and print a number. This one behaves like an actual tracking radar: it **searches** at full sweep speed, **locks on** the moment it detects something in range, **dwells** on that bearing to confirm and track it, then **resumes the sweep** — deliberately, even if the object is still sitting there.
 
That last part matters. A naive implementation gets stuck staring at the first thing it sees forever. This one is built specifically so it can't be pinned down by a static decoy while something else moves through an unwatched angle — the sweep always completes, every pass.
 
## ⚙️ How It Works
 
**Search mode** — the servo sweeps 0°–180° continuously, pinging the ultrasonic sensor at each step and classifying every reading into a zone (A–D) and a status (`CLEAR` / `MONITOR` / `ALERT`).
 
**Track mode** — the instant a reading crosses into `ALERT` range, the servo stops sweeping and holds that exact bearing, re-measuring continuously for a fixed dwell window. The buzzer's beep rate accelerates the closer the object gets — communicating urgency through information, not just noise.
 
**Resume** — when the dwell window ends, the system forcibly advances past that bearing and returns to full-speed search, regardless of whether the object is still there. No blind spots, no infinite lock-on.
 
Every reading — angle, distance, zone, status, timestamp — is pushed live over WiFi to a Node backend and rendered on a browser dashboard in real time.
 
## ✨ Features
 
- 🔄 Continuous 0°–180° servo sweep with configurable step size and settle time
- 🎯 Automatic search → track → resume state machine
- 🔊 Distance-proportional buzzer escalation (faster beeps = closer object)
- 🗺️ Angular zone classification (A–D) for spatial context
- 📡 Live WiFi telemetry via HTTP POST, JSON payloads
- 🖥️ Real-time web dashboard
- 🛡️ Anti-decoy design — sweep can't be permanently pinned on one bearing
## 🔧 Hardware
 
| Component | Role |
|---|---|
| ESP32 Dev Board | Main controller — WiFi + sweep logic |
| HC-SR04 Ultrasonic Sensor | Distance sensing |
| SG90 Micro Servo | Sweep mechanism |
| Active Buzzer | Proximity alert |
 
## 🧱 Tech Stack
 
- **Firmware:** C++ (Arduino core for ESP32), ESP32Servo, ArduinoJson, HTTPClient
- **Backend:** Node.js
- **Dashboard:** HTML/CSS/JS
## 📁 Project Structure
 
```
esp32-radar-scanner/
├── esp32-radar-scanner.ino     # Firmware — sweep, track, buzzer, WiFi POST
└── perimeter-monitor/
    ├── backend/                 # Node server — receives & serves scan data
    └── dashboard/                # Web dashboard — live visualization
```
 
## 🚀 Getting Started
 
### Firmware
1. Open `esp32-radar-scanner.ino` in Arduino IDE
2. Install the ESP32 board package, plus the `ESP32Servo` and `ArduinoJson` libraries
3. Set your own `WIFI_SSID`, `WIFI_PASSWORD`, and `SERVER_URL` (your machine's LAN IP) — **don't commit real credentials**
4. Select your board and COM port, then upload (115200 baud recommended for reliable flashing)
### Backend
```bash
cd perimeter-monitor/backend
npm install
npm start
```
 
### Dashboard
Open `perimeter-monitor/dashboard` in your browser once the backend is running and the ESP32 is connected to WiFi.
 
## 📸 Demo
 
<img width="1365" height="688" alt="image" src="https://github.com/user-attachments/assets/25799379-6aa9-44f6-806b-d3a19cf000be" />

 
## 🗺️ Roadmap
 
- [ ] Configurable alert thresholds from the dashboard
- [ ] Multi-object tracking across sweep passes
- [ ] Persistent scan history/logging
## 👤 Author
 
**Toluwanimi Alfred**
Mechatronics Engineering student, FUTMINNA · AgriTech builder
 
- GitHub: [@alfredoeinsteino2024](https://github.com/alfredoeinsteino2024)
- LinkedIn: [toluwanimialfred](https://linkedin.com/in/toluwanimialfred/)
- X: [@AlfredFadipe](https://x.com/AlfredFadipe)
## 📄 License
 
This project is licensed under the [MIT License](https://opensource.org/licenses/MIT) — free to use, modify, and distribute. Swap this section out if you'd rather use a different license or keep it closed.
 
