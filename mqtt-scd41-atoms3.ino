#include <M5Unified.h>
#include <M5UnitENV.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#include "config.h"

WiFiClient wifi_client;
PubSubClient mqtt_client(mqtt_host, mqtt_port, NULL, wifi_client);

SCD4X scd4x;

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 9600;
  M5.begin(cfg);

  M5.Display.setTextSize(3);
  M5.Display.setBrightness(32);


  // Wifi
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.setSleep(false);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    switch (count % 4) {
      case 0:
        Serial.println("|");
        M5.Display.fillRect(0, 0, 16, 16, TFT_YELLOW);
        break;
      case 1:
        Serial.println("/");
        break;
      case 2:
        M5.Display.fillRect(0, 0, 16, 16, TFT_BLACK);
        Serial.println("-");
        break;
      case 3:
        Serial.println("\\");
        break;
    }
    count++;
    if (count >= 240) reboot();  // 240 / 4 = 60sec
  }
  M5.Display.fillRect(0, 0, 16, 16, TFT_YELLOW);
  Serial.println("WiFi connected!");
  delay(1000);

  // MQTT
  bool rv = false;
  if (mqtt_use_auth == true) {
    rv = mqtt_client.connect(mqtt_client_id, mqtt_username, mqtt_password);
  } else {
    rv = mqtt_client.connect(mqtt_client_id);
  }
  if (rv == false) {
    Serial.println("mqtt connecting failed...");
    reboot();
  }
  Serial.println("MQTT connected!");
  delay(1000);

  M5.Display.fillRect(0, 0, 16, 16, TFT_BLUE);

  // configTime
  configTime(9 * 3600, 0, "ntp.nict.jp");
  struct tm t;
  if (!getLocalTime(&t)) {
    Serial.println("getLocalTime() failed...");
    delay(1000);
    reboot();
  }
  Serial.println("configTime() success!");
  M5.Display.fillRect(0, 0, 16, 16, TFT_GREEN);

  delay(1000);


  if (!scd4x.begin(&Wire, SCD4X_I2C_ADDR, 2u, 1u, 400000U)) {
    Serial.println("Couldn't find SCD4X");
    ESP.restart();
  }
}

void reboot() {
  Serial.println("REBOOT!!!!!");
  for (int i = 0; i < 30; ++i) {
    M5.Display.fillRect(0, 0, 16, 16, TFT_MAGENTA);
    delay(100);
    M5.Display.fillRect(0, 0, 16, 16, TFT_BLACK);
    delay(100);
  }

  ESP.restart();
}

void loop() {

  mqtt_client.loop();
  if (!mqtt_client.connected()) {
    Serial.println("MQTT disconnected...");
    reboot();
  }

  M5.update();

  if (scd4x.update()) {
    int co2 = scd4x.getCO2();
    int tmp = (int)scd4x.getTemperature(); // float
    int hum = (int)scd4x.getHumidity(); // float

    Serial.print(F("CO2(ppm):"));
    Serial.print(co2);

    Serial.print(F(", Temperature(C):"));
    Serial.print(tmp);

    Serial.print(F(", Humidity(%RH):"));
    Serial.print(hum);

    Serial.println();


    M5.Display.drawString("SCD41", 0, 20);

    char buf[256];

    snprintf(buf, 255, "%dppm  ", co2);
    M5.Display.drawString(buf, 0, 45);

    snprintf(buf, 255, "T:%dC  ", tmp);
    M5.Display.drawString(buf, 0, 70);

    snprintf(buf, 255, "H:%d%%  ", hum);
    M5.Display.drawString(buf, 0, 95);
  }

  delay(1000);
}
