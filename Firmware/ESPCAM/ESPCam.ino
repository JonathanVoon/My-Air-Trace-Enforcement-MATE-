#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";


WebServer server(80);

// AI Thinker ESP32-CAM pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_PIN 4

void handleCapture()
{
  Serial.println("Capture request received");

  // Turn flash ON
  digitalWrite(FLASH_PIN, HIGH);
  Serial.println("Flash ON");

  delay(200);  // allow LED to brighten


  camera_fb_t * fb = esp_camera_fb_get();


  // Turn flash OFF immediately after capture
  digitalWrite(FLASH_PIN, LOW);
  Serial.println("Flash OFF");


  if (!fb)
  {
    Serial.println("Camera capture failed");
    server.send(500, "text/plain", "Camera failed");
    return;
  }


  server.sendHeader("Access-Control-Allow-Origin", "*");

  server.send_P(
    200,
    "image/jpeg",
    (const char *)fb->buf,
    fb->len
  );


  Serial.print("Image size: ");
  Serial.println(fb->len);


  esp_camera_fb_return(fb);
}


void setup()
{
  Serial.begin(115200);

  Serial.println("ESP32-CAM Starting");


  camera_config_t config;
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;

  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;


  if(psramFound())
  {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }


  if(esp_camera_init(&config) != ESP_OK)
  {
    Serial.println("Camera init failed");
    return;
  }


  WiFi.begin(ssid,password);

  Serial.print("Connecting WiFi");

  while(WiFi.status()!=WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }


  Serial.println();
  Serial.println("WiFi Connected");

  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());


  server.on("/capture", HTTP_GET, handleCapture);

  server.begin();

  Serial.println("Camera Server Ready");
}


void loop()
{
  server.handleClient();
}
