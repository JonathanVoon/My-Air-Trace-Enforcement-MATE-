#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <WebServer.h>

#define SDS_RX 18
#define SDS_TX 17
#define GPS_RX 15
#define GPS_TX 16
#define LORA_SCK  12
#define LORA_MISO 13
#define LORA_MOSI 11
#define LORA_SS   10
#define LORA_RST  14
#define LORA_DIO0 21
#define SERVO_PIN 9
#define LORA_FREQUENCY 433E6

HardwareSerial SDS(1);
HardwareSerial GPS(2);
TinyGPSPlus gps;
Servo cameraServo;
WebServer server(80);

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const char* cameraURL = "http://YOUR_ESP32_CAM_IP/capture";

const int mod_thres = 1600;
const int bad_thres = 1800;

enum SDSState { WAIT_AA, READ_REST } sdsState = WAIT_AA;
int sdsIndex = 0;
uint8_t sdsBuffer[10];
float pm25 = 0.0;
float pm10 = 0.0;

int node1RawValue = 0;
String node1Status = "GOOD";
int node2RawValue = 0;
String node2Status = "GOOD";

unsigned long lastPrintTime = 0;
const unsigned long printInterval = 5000;

unsigned long lastCaptureTime = 0;
const unsigned long captureCooldown = 30000;

void readSDS011();
void readLoRa();
void captureCamera();
void printSystemStatus();
String categorizeAQ(int rawValue);
void handleSensorRequest();

void initializeServo() {
  Serial.println("---------------------------------------");
  Serial.println("Initializing Camera Servo...");

  cameraServo.attach(SERVO_PIN);

  // Move to center
  Serial.println("Moving servo to CENTER (90°)");
  cameraServo.write(90);
  delay(1500);

  // Optional self-test
  Serial.println("Testing LEFT...");
  cameraServo.write(30);
  delay(1000);

  Serial.println("Testing RIGHT...");
  cameraServo.write(150);
  delay(1000);

  Serial.println("Returning to CENTER...");
  cameraServo.write(90);
  delay(1500);

  Serial.println("Servo Initialization Complete");
  Serial.println("----------------------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("MATE Main Node (ESP32-S3) Initializing");
  Serial.println("----------------------------------------");

  // Initialize and test servo FIRST
  initializeServo();

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - wifiStart > 15000) {
      Serial.println("\n Connection timed out.");
      break;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n WiFi Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Dashboard fetches this endpoint every 3s to display live PM2.5/PM10 readings.
    server.on("/sensor", handleSensorRequest);
    server.begin();
    Serial.println("HTTP server started - /sensor endpoint ready");
  } else {
    Serial.println("\nWiFi Failed to Connect");
  }

  SDS.begin(9600, SERIAL_8N1, SDS_RX, SDS_TX);
  GPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa initialization failed. Check wiring.");
    while (1) {
      delay(1000);
    }
  }  
  cameraServo.attach(SERVO_PIN);
  cameraServo.write(90);
  
  Serial.println("System Ready. Starting Main Loop...");
}

void loop() {
  server.handleClient();


  while (GPS.available()) {
    gps.encode(GPS.read());
  }
  
  readSDS011();
  readLoRa();

  if (millis() - lastPrintTime >= printInterval) {
    lastPrintTime = millis();
    printSystemStatus();
  }
}

void readSDS011() {
  while (SDS.available() > 0) {
    byte b = SDS.read();
    
    if (sdsState == WAIT_AA) {
      if (b == 0xAA) {
        sdsBuffer[0] = 0xAA;
        sdsIndex = 1;
        sdsState = READ_REST;
      }
    } 
    else if (sdsState == READ_REST) {
      sdsBuffer[sdsIndex++] = b;
      
      if (sdsIndex == 10) {
        if (sdsBuffer[1] == 0xC0 && sdsBuffer[9] == 0xAB) {
          byte checksum = 0;
          for (int i = 2; i < 8; i++) {
            checksum += sdsBuffer[i];
          }
          
          if (checksum == sdsBuffer[8]) {
            pm25 = (sdsBuffer[2] + (sdsBuffer[3] << 8)) / 10.0;
            pm10 = (sdsBuffer[4] + (sdsBuffer[5] << 8)) / 10.0;
          }
        }
        sdsState = WAIT_AA;
      }
    }
  }
}

void readLoRa() {
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    Serial.print("LoRa packet received, size: ");
    Serial.println(packetSize);
    
    String packet = "";
    while (LoRa.available()) {
      packet += (char)LoRa.read();
    }
    packet.trim();
    
    Serial.print("LoRa payload: ");
    Serial.println(packet);

    int firstComma = packet.indexOf(',');
    
    if (firstComma > 0) {
      String nodeId = packet.substring(0, firstComma);
      
      int secondComma = packet.indexOf(',', firstComma + 1);
      String valueStr = "";
      
      if (secondComma > 0) {
        valueStr = packet.substring(firstComma + 1, secondComma);
      } else {
        valueStr = packet.substring(firstComma + 1);
      }
      
      int rawValue = valueStr.toInt();

      if (rawValue < 0) rawValue = 0;

      if (nodeId == "BLACKSENTRY") {
        node1RawValue = rawValue;
        node1Status = categorizeAQ(rawValue);
        Serial.println("Updated Node 1 data");
      } 
      else if (nodeId == "BLUESENTRY") {
        node2RawValue = rawValue;
        node2Status = categorizeAQ(rawValue);
        Serial.println("Updated Node 2 data");
      } 
      else {
        Serial.print("Unknown Node ID: ");
        Serial.println(nodeId);
      }
    } else {
      Serial.println("Malformed LoRa packet. Expected format: NODE<X>,<value>");
    }
  }
}

void captureCamera() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    Serial.println("Sending HTTP GET to ESP32-CAM...");
    
    http.begin(cameraURL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpResponseCode = http.GET();
    Serial.print("Camera HTTP Response Code: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode > 0) {
      int imageSize = http.getSize();
      Serial.print("Image size: ");
      Serial.print(imageSize);
      Serial.println(" bytes");
    } else {
      Serial.print("HTTP GET failed, error: ");
      Serial.println(http.errorToString(httpResponseCode));
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot trigger camera.");
  }
}

String categorizeAQ(int rawValue) {
  if (rawValue < mod_thres) {
    return "GOOD";
  } else if (rawValue < bad_thres) {
    return "MODERATE";
  } else {
    return "BAD";
  }
}

// Serves the local SDS011 PM2.5/PM10 readings as JSON so the web dashboard
// can poll it directly (dashboard fetches this every 3 seconds).
void handleSensorRequest() {
  server.sendHeader("Access-Control-Allow-Origin", "*"); // allow the browser page to read this response

  bool badAirNow = (pm25 > 35.0 || node1Status == "BAD" || node2Status == "BAD");

  String json = "{";
  json += "\"pm25\":" + String(pm25, 1) + ",";
  json += "\"pm10\":" + String(pm10, 1) + ",";
  json += "\"node1Value\":" + String(node1RawValue) + ",";
  json += "\"node1Status\":\"" + node1Status + "\",";
  json += "\"node2Value\":" + String(node2RawValue) + ",";
  json += "\"node2Status\":\"" + node2Status + "\",";
  if (gps.location.isValid()) {
    json += "\"lat\":" + String(gps.location.lat(), 6) + ",";
    json += "\"lng\":" + String(gps.location.lng(), 6) + ",";
  }
  json += "\"badAir\":" + String(badAirNow ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

void printSystemStatus() {
  Serial.println("System Status Report");
  Serial.println("---------------------------------------");
  
  Serial.print("PM2.5: ");
  Serial.println(pm25);
  Serial.print("PM10: ");
  Serial.println(pm10);
  
  if (gps.location.isValid()) {
    Serial.print("GPS Lat: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("GPS Lng: ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("GPS: Waiting for fix...");
  }
  
  Serial.print("Node 1 Value: ");
  Serial.print(node1RawValue);
  Serial.print(" (");
  Serial.print(node1Status);
  Serial.println(")");
  
  Serial.print("Node 2 Value: ");
  Serial.print(node2RawValue);
  Serial.print(" (");
  Serial.print(node2Status);
  Serial.println(")");
  
  int difference = node1RawValue - node2RawValue;
  int servoAngle = 90 - (difference/5);
   
  servoAngle = constrain(servoAngle, 0, 180);
  
  cameraServo.write(servoAngle);
  Serial.print("Servo Angle: ");
  Serial.println(servoAngle);
  
  Serial.println("----------------------------------------");
  
  bool badAir = (pm25 > 35.0 || node1Status == "BAD" || node2Status == "BAD");


  if (badAir) {

    Serial.println("Illegal burning detected");

    if (millis() - lastCaptureTime > captureCooldown) {

      Serial.println("Taking evidence photo...");

      captureCamera();

      lastCaptureTime = millis();

    }
    else {

      Serial.println("Photo cooldown active...");
    }

  }
  else {

    Serial.println("Air quality normal");

    lastCaptureTime = 0;
  }
}
