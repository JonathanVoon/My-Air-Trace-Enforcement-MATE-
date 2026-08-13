#include <SPI.h>
#include <LoRa.h>

// PIN DEFINITIONS FOR LORA (ESP32-C3 MINI)
#define LORA_SCK  4
#define LORA_MISO 5
#define LORA_MOSI 6
#define LORA_SS   7
#define LORA_RST  10
#define LORA_DIO0 3

// PIN DEFINITIONS FOR MQ135
#define MQ135_ANALOG_PIN 2  // GPIO 2 is ADC1_CH2

#define FREQUENCY 433E6

int messageCounter = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32-C3 Transmitter + MQ135 Initializing...");

  // Configure ADC attenuation for 0-3.3V range (ESP32-C3 default)
  analogSetAttenuation(ADC_11db); 

  // Initialize standard hardware SPI for the ESP32-C3
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);

  // Set the LoRa pins
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(FREQUENCY)) {
    Serial.println("LoRa Initialization Failed!");
    while (1);
  }

  Serial.println("System Ready - Transmitting Air Quality Thresholds...");
}

void loop() {
  // Read raw analog value from MQ135 (0 to 4095)
  int rawGasValue = analogRead(MQ135_ANALOG_PIN);
  
  // Determine air quality status string based on raw thresholds
  String airQualityStatus = "";

  if (rawGasValue <= 1400) {
    airQualityStatus = "GOOD";
  } 
  else if (rawGasValue >= 1400 && rawGasValue < 1700) {
    airQualityStatus = "MODERATE";
  } 
  else { // 1800 or above
    airQualityStatus = "BAD";
  }

  // Print local diagnostics to the C3 Serial Monitor
  Serial.print("Packet ID: ");
  Serial.print(messageCounter);
  Serial.print(" | Raw Value: ");
  Serial.print(rawGasValue);
  Serial.print(" | Status: ");
  Serial.println(airQualityStatus);

  // Broadcast data over LoRa to the S3 Receiver
  LoRa.beginPacket();
  LoRa.print("Air: ");
  LoRa.print(airQualityStatus);
  // Optional: keeping raw value inside the packet if you want to track minor drifts
  LoRa.print(" (");
  LoRa.print(rawGasValue);
  LoRa.print(") | ID: ");
  LoRa.print(messageCounter);
  LoRa.endPacket();

  messageCounter++;

  // Wait 3 seconds between transmissions
  delay(3000);
}
