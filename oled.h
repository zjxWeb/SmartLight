#pragma once
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <NTPClient.h>
#include "config.h"
#include "lunar.h"
#include "weather.h"
#include "led.h"

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

#define PAGE_COUNT 4
static uint8_t oledPage = 0;

// 前置声明
void syncOLEDBrightness();

void oledBegin() {
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();
}

// ===================================================
//  开机动画
// ===================================================
void bootAnimation() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  const char* title = "SmartLight";
  for (int i = 0; i <= (int)strlen(title); i++) {
    display.clearDisplay();
    display.setCursor(4, 8);
    for (int j = 0; j < i; j++) display.print(title[j]);
    if (i < (int)strlen(title))
      display.fillRect(4 + i * 12, 8, 10, 16, SSD1306_WHITE);
    display.display();
    delay(80);
  }
  display.setTextSize(1);
  display.setCursor(14, 32);
  display.print("Qingyang, Gansu");
  display.setCursor(28, 46);
  display.print("Loading...");
  display.display();
  delay(400);
  display.drawRect(4, 54, 120, 8, SSD1306_WHITE);
  display.display();
  for (int w = 0; w <= 116; w += 4) {
    display.fillRect(6, 56, w, 4, SSD1306_WHITE);
    display.display();
    delay(15);
  }
  delay(300);
}

// ===================================================
//  WiFi连接动画
// ===================================================
void connectWiFiAnim() {
  uint8_t dots = 0, spin = 0;
  const char spinChars[] = {'-', '\\', '|', '/'};
  while (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(16, 18);
    display.print("Connecting WiFi");
    for (uint8_t i = 0; i < dots; i++) display.print(".");
    dots = (dots + 1) % 4;
    display.setTextSize(2);
    display.setCursor(56, 36);
    display.print(spinChars[spin++ % 4]);
    display.display();
    delay(350);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(24, 18);
  display.print("WiFi Connected!");
  display.setCursor(16, 32);
  display.print(WiFi.localIP().toString());
  display.setCursor(4, 48);
  display.print("http://");
  display.print(WiFi.localIP().toString());
  display.display();
  delay(2500);
}

// ===================================================
//  epoch → 公历
// ===================================================
void epochToDate(unsigned long ep, int &yy, int &mm, int &dd) {
  unsigned long days = ep / 86400UL;
  for (yy = 1970;;yy++) {
    int yd = ((yy%4==0&&yy%100!=0)||(yy%400==0)) ? 366 : 365;
    if (days < (unsigned long)yd) break;
    days -= yd;
  }
  int md[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
  if ((yy%4==0&&yy%100!=0)||(yy%400==0)) md[2] = 29;
  for (mm = 1; mm <= 12; mm++) {
    if (days < (unsigned long)md[mm]) { dd = days + 1; break; }
    days -= md[mm];
  }
}

// ===================================================
//  各页绘制
// ===================================================
void drawPage(uint8_t page, int xOff, NTPClient& timeClient,
              float temperature, float humidity) {
  display.setTextColor(SSD1306_WHITE);

  switch (page) {

    // ---- 页0：时钟 + 日期 + 农历 ----
    case 0: {
      String ts = timeClient.getFormattedTime().substring(0, 5);
      display.setTextSize(4);
      display.setCursor(xOff, 0);
      display.print(ts);

      // 秒进度条
      int sec = timeClient.getSeconds();
      display.drawFastHLine(xOff, 33, 128, SSD1306_WHITE);
      display.fillRect(xOff, 33, (int)(128.0 * sec / 60), 2, SSD1306_WHITE);

      // 公历 + 星期
      int yy, mm, dd;
      epochToDate(timeClient.getEpochTime(), yy, mm, dd);
      const char* wd[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
      display.setTextSize(1);
      display.setCursor(xOff, 36);
      if (mm < 10) display.print("0"); display.print(mm);
      display.print("/");
      if (dd < 10) display.print("0"); display.print(dd);
      display.print("/"); display.print(yy);
      display.print("  "); display.print(wd[timeClient.getDay()]);

      // 农历（拆成两段防止超宽）
      LunarDate lunar = solarToLunar(yy, mm, dd);
      display.setCursor(xOff, 46);
      display.print(lunar.monthStr);
      display.print(".");
      display.print(lunar.dayStr);
      display.print("  ");
      display.print(lunar.ganZhi);
      // 生肖放右侧，不换行
      display.setCursor(xOff + 90, 46);
      display.print(lunar.zodiac);
      break;
    }

    // ---- 页1：室内温湿度 ----
    case 1: {
      display.setTextSize(4);
      display.setCursor(xOff, 0);
      display.print((int)temperature);
      display.setTextSize(2);
      display.print(".");
      display.print((int)(temperature * 10) % 10);
      display.print("C");

      display.drawFastHLine(xOff, 33, 128, SSD1306_WHITE);

      display.setTextSize(2);
      display.setCursor(xOff, 36);
      display.print((int)humidity);
      display.print("% RH");

      // 舒适度小字，Y=55保证不超出
      display.setTextSize(1);
      display.setCursor(xOff, 55);
      if (temperature >= 18 && temperature <= 26 && humidity >= 40 && humidity <= 70)
        display.print("Comfort: Good");
      else if (temperature > 30)
        display.print("Comfort: Hot!");
      else if (temperature < 10)
        display.print("Comfort: Cold!");
      else
        display.print("Comfort: Fair");
      break;
    }

    // ---- 页2：今日天气 ----
    case 2: {
      display.setTextSize(4);
      display.setCursor(xOff, 0);
      display.print(weatherTemp);
      display.setTextSize(2);
      display.print("C");

      display.drawFastHLine(xOff, 33, 128, SSD1306_WHITE);

      display.setTextSize(2);
      display.setCursor(xOff, 36);
      display.print(weatherToEn(weatherText));
      break;
    }

    // ---- 页3：灯状态 ----
    case 3: {
      display.setTextSize(4);
      display.setCursor(xOff, 0);
      display.print(lightOn ? "ON" : "OFF");

      display.drawFastHLine(xOff, 33, 128, SSD1306_WHITE);

      display.setTextSize(1);
      display.setCursor(xOff, 36);
      display.print("Bright: ");
      display.print(brightness);
      display.print("%");
      display.drawRect(xOff, 45, 110, 6, SSD1306_WHITE);
      display.fillRect(xOff, 45, (int)(110.0 * brightness / 100), 6, SSD1306_WHITE);

      // 模式，Y=55不超出
      display.setCursor(xOff, 55);
      display.print("Mode: ");
      display.print(lightMode);
      break;
    }
  }
}

// ===================================================
//  滑入动画
// ===================================================
void slideInPage(uint8_t page, NTPClient& timeClient,
                 float temperature, float humidity) {
  for (int offset = 128; offset >= 0; offset -= 16) {
    display.clearDisplay();
    drawPage(page, offset, timeClient, temperature, humidity);
    display.display();
    delay(10);
  }
}

void updateOLED(NTPClient& timeClient, float temperature, float humidity) {
  syncOLEDBrightness();
  slideInPage(oledPage, timeClient, temperature, humidity);
  oledPage = (oledPage + 1) % PAGE_COUNT;
}

// 单独同步OLED亮度，不切页（供loop实时调用）
void syncOLEDBrightness() {
  if (!lightOn) {
    // 灯灭：dim模式（最暗但不关屏）
    display.dim(true);
  } else {
    display.dim(false);
    // 亮度映射：brightness 0-100 → contrast 0-255
    uint8_t contrast = map(brightness, 0, 100, 0, 255);
    // SSD1306完整对比度设置序列
    display.ssd1306_command(0x81);
    display.ssd1306_command(contrast);
    display.ssd1306_command(0xD9);   // 预充电周期
    uint8_t precharge = (brightness > 50) ? 0xF1 : 0x11;
    display.ssd1306_command(precharge);
  }
}
