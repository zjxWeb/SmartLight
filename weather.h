#pragma once
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "config.h"

// ===== 当前天气 =====
String weatherText = "--";
String weatherTemp = "--";
String weatherHumi = "--";

// ===== 3天预报 =====
struct DayForecast {
  char date[6];    // "03/24"
  char high[4];    // "18"
  char low[4];     // "5"
  char text[12];   // "Sunny"
};
DayForecast forecast[3];

// 中文天气→英文
String weatherToEn(String cn) {
  if (cn=="晴")       return "Sunny";
  if (cn=="多云")     return "Cloudy";
  if (cn=="阴")       return "Overcast";
  if (cn=="小雨")     return "Lt.Rain";
  if (cn=="中雨")     return "Rain";
  if (cn=="大雨")     return "Hvy.Rain";
  if (cn=="暴雨")     return "Storm";
  if (cn=="雷阵雨")   return "Thunder";
  if (cn=="阵雨")     return "Shower";
  if (cn=="小雪")     return "Lt.Snow";
  if (cn=="中雪")     return "Snow";
  if (cn=="大雪")     return "Hvy.Snow";
  if (cn=="暴雪")     return "Blizzard";
  if (cn=="雨夹雪")   return "Sleet";
  if (cn=="雾")       return "Foggy";
  if (cn=="霾")       return "Haze";
  if (cn=="沙尘暴")   return "Sandstorm";
  if (cn=="浮尘")     return "Dusty";
  if (cn=="扬沙")     return "Blowing Sand";
  if (cn=="冻雨")     return "Frz.Rain";
  return cn.substring(0, 8);
}

// 获取当前天气
void fetchWeatherNow() {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, WEATHER_NOW_URL);
  if (http.GET() == 200) {
    StaticJsonDocument<512> doc;
    if (!deserializeJson(doc, http.getString())) {
      weatherText = doc["results"][0]["now"]["text"].as<String>();
      weatherTemp = doc["results"][0]["now"]["temperature"].as<String>();
    }
  }
  http.end();
}

// 获取3天预报
void fetchWeather3Day() {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, WEATHER_3D_URL);
  if (http.GET() == 200) {
    StaticJsonDocument<1024> doc;
    if (!deserializeJson(doc, http.getString())) {
      JsonArray daily = doc["results"][0]["daily"];
      for (int i = 0; i < 3 && i < (int)daily.size(); i++) {
        // date: "2026-03-24" → "03/24"
        String d = daily[i]["date"].as<String>();
        snprintf(forecast[i].date, 6, "%s/%s",
                 d.substring(5,7).c_str(), d.substring(8,10).c_str());
        snprintf(forecast[i].high, 4, "%s", daily[i]["high"].as<String>().c_str());
        snprintf(forecast[i].low,  4, "%s", daily[i]["low"].as<String>().c_str());
        String en = weatherToEn(daily[i]["text_day"].as<String>());
        snprintf(forecast[i].text, 12, "%s", en.c_str());
      }
    }
  }
  http.end();
}

void fetchAllWeather() {
  fetchWeatherNow();
  fetchWeather3Day();
}
