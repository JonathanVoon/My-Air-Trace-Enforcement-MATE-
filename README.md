# MATE — My Air Trace & Enforcement

Solar-Powered IoT Mesh Network & Automated Air Quality Enforcement System  
Developed by Marvel Kids — Group 87 | Engineering Team Project (Universiti Teknologi PETRONAS)

---

## Project Overview

MATE (My Air Trace & Enforcement) is an automated, solar-assisted IoT surveillance and air quality monitoring framework designed for high-risk zones subject to illegal open burning and industrial emissions. 

By combining gas sensing, directional detection, camera telemetry, and LoRa mesh networking, MATE pinpoints pollution spikes and instantly transmits visual and spatial evidence back to a dispatcher dashboard.

---

## System Architecture

The project consists of three hardware firmware nodes and a centralized web dashboard:

1. Main Node (Firmware/MainNode/)  
   * Controller: ESP32-S3  
   * Role: Central gateway node. Captures gas readings (MQ135), drives warning actuators/servos, handles LoRa/Wi-Fi communications, and dispatches real-time alerts.
   
2. Sentry Node (Firmware/SentryNode/)  
   * Role: Perimeter surveillance / directional transmitter. Detects localized particulate threshold breaches and sends wireless telemetry back to the Main Hub.

3. ESP32-CAM Node (Firmware/ESPCam/)  
   * Role: Automated evidence logging. Captures high-resolution visual evidence of smoke plumes upon receiving spike triggers and serves snapshots over HTTP.

4. Web Dispatcher Dashboard (Dashboard/)  
   * Role: Live monitoring console built with HTML5, Tailwind CSS, and JavaScript. Displays real-time sensor metrics, radar telemetry, alert logs, and evidence photos.

---

## Repository Structure

My-Air-Trace-Enforcement-MATE-/
├── .gitignore               # Excludes build binaries & local credentials
├── README.md                # Project documentation
├── Dashboard/
│   └── Dashboard.html       # Live Dispatcher Enforcement Dashboard
└── Firmware/
    ├── MainNode/
    │   └── MainNode.ino     # Gateway firmware (MQ135, LoRa, Servo, WebServer)
    ├── SentryNode/
    │   └── SentryNode.ino   # Remote transmitter firmware
    └── ESPCam/
        └── ESPCAM.ino       # Camera capture firmware
