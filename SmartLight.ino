#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "led.h"
#include "weather.h"
#include "lunar.h"
#include "oled.h"
#include "webserver.h"

// ===== 核心对象 =====
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);
WiFiUDP      ntpUDP;
NTPClient    timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET, 60000);
DHT          dht(PIN_DHT, DHT11);

float temperature = 0, humidity = 0;

// 启动保护期：上电后5秒内忽略MQTT retain消息，防止旧的off指令关灯
#define MQTT_PROTECT_MS 5000
unsigned long startupTime = 0;

// ===== 定时器 =====
unsigned long lastDHT        = 0;
unsigned long lastWeather    = 0;
unsigned long lastOLED       = 0;
unsigned long lastReport     = 0;
unsigned long lastHistory    = 0;
unsigned long lastBrightSync = 0;

// ===================================================
//  WiFi 连接
// ===================================================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  connectWiFiAnim();  // oled.h 中的动画
}

// ===================================================
//  OTA 初始化
// ===================================================
void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 20);
    display.print("OTA Updating...");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int pct = progress / (total / 100);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(20, 16);
    display.print("OTA Updating...");
    display.setCursor(52, 30);
    display.print(pct); display.print("%");
    display.drawRect(4, 44, 120, 10, SSD1306_WHITE);
    display.fillRect(6, 46, (int)(116.0*pct/100), 6, SSD1306_WHITE);
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    display.clearDisplay();
    display.setCursor(24, 24);
    display.print("OTA Done! Reboot");
    display.display();
    delay(1000);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    display.clearDisplay();
    display.setCursor(20, 24);
    display.print("OTA Error: ");
    display.print(error);
    display.display();
    delay(2000);
  });

  ArduinoOTA.begin();
}

// ===================================================
//  巴法云 MQTT 回调
//  灯泡002: off / on / on#80 / on#80#16711680
// ===================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // 启动保护期内忽略所有消息（防止retain旧指令）
  if (millis() - startupTime < MQTT_PROTECT_MS) return;

  String msg = "";
  for (unsigned int i=0; i<length; i++) msg += (char)payload[i];
  msg.trim();

  // ---- 灯泡主题：开关/亮度/颜色 ----
  if (String(topic) == TOPIC_LIGHT) {
    if (msg == "off") {
      setLightOn(false);
      return;
    }
    if (msg.startsWith("on")) {
      setLightOn(true);
      int s1 = msg.indexOf('#');
      int s2 = (s1 >= 0) ? msg.indexOf('#', s1 + 1) : -1;
      if (s1 > 0) {
        String bStr = (s2 > 0) ? msg.substring(s1+1, s2) : msg.substring(s1+1);
        brightness = constrain(bStr.toInt(), 0, 100);
      }
      if (s2 > 0) {
        String colorStr = msg.substring(s2 + 1);
        if (colorStr.indexOf(',') >= 0) {
          int c1 = colorStr.indexOf(',');
          int c2 = colorStr.indexOf(',', c1 + 1);
          uint8_t cr = colorStr.substring(0, c1).toInt();
          uint8_t cg = colorStr.substring(c1+1, c2).toInt();
          uint8_t cb = colorStr.substring(c2+1).toInt();
          rgbColor = ((uint32_t)cr << 16) | ((uint32_t)cg << 8) | cb;
        } else {
          rgbColor = (uint32_t)colorStr.toInt();
        }
        lightMode = "custom";
      }
    }
    return;
  }

  if (String(topic) == TOPIC_MODE) {
    if (msg == "on")  { setLightOn(true);  return; }
    if (msg == "off") { setLightOn(false); return; }
    lightMode = msg;
    setLightOn(true);
    return;
  }
}

void connectMQTT() {
  mqtt.setServer(BEMFA_HOST, BEMFA_PORT);
  mqtt.setCallback(mqttCallback);
  while (!mqtt.connected()) {
    if (mqtt.connect(BEMFA_UID)) {
      mqtt.subscribe(TOPIC_LIGHT);
      mqtt.subscribe(TOPIC_MODE);
    } else {
      delay(3000);
    }
  }
}

void reportSensor() {
  if (!mqtt.connected()) return;
  String payload = "#" + String((int)temperature) + "#" + String((int)humidity);
  mqtt.publish(TOPIC_SENSOR, payload.c_str());
}

// ===================================================
//  setup
// ===================================================
void setup() {
  Serial.begin(115200);

  // 关闭板载LED（低电平亮，所以设HIGH=灭）
  pinMode(PIN_BUILTIN_LED1, OUTPUT);
  digitalWrite(PIN_BUILTIN_LED1, HIGH);
  pinMode(PIN_BUILTIN_LED2, OUTPUT);
  digitalWrite(PIN_BUILTIN_LED2, HIGH);

  dht.begin();
  ledBegin();
  oledBegin();

  bootAnimation();
  connectWiFi();

  timeClient.begin();
  timeClient.update();

  setupOTA();
  webServerBegin();
  connectMQTT();

  // 首次数据获取
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;
  pushHistory(temperature, humidity);

  fetchAllWeather();
  startupTime = millis();  // 记录启动时间，用于MQTT保护期
}

// ===================================================
//  loop
// ===================================================
void loop() {
  ArduinoOTA.handle();
  webServerHandle();

  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  unsigned long now = millis();

  // 每2秒读DHT11
  if (now - lastDHT > 2000) {
    lastDHT = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperature = t;
    if (!isnan(h)) humidity = h;
  }

  // 每5分钟记录一次历史（折线图）
  if (now - lastHistory > 300000) {
    lastHistory = now;
    pushHistory(temperature, humidity);
  }

  // 每60秒上报传感器
  if (now - lastReport > 60000) {
    lastReport = now;
    reportSensor();
  }

  // 每10分钟拉取天气
  if (now - lastWeather > 600000) {
    lastWeather = now;
    fetchAllWeather();
  }

  // 每秒同步OLED亮度
  if (now - lastBrightSync > 1000) {
    lastBrightSync = now;
    syncOLEDBrightness();
  }

  // OLED切换（由config.h中OLED_INTERVAL控制，默认6秒）
  if (now - lastOLED > OLED_INTERVAL) {
    lastOLED = now;
    timeClient.update();
    updateOLED(timeClient, temperature, humidity);
  }

  updateLED(temperature);
}
