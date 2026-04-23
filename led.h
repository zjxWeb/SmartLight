#pragma once
#include <Adafruit_NeoPixel.h>
#include "config.h"

Adafruit_NeoPixel strip(LED_COUNT, PIN_LED, NEO_GRB + NEO_KHZ800);

bool     lightOn    = false;
uint8_t  brightness = 80;
uint32_t rgbColor   = 0xFFFFFF;
String   lightMode  = "white";

static uint16_t effectStep = 0;
static uint8_t  breathVal  = 0;
static bool     breathUp   = true;
static unsigned long lastEffect   = 0;
static unsigned long lightOnStart = 0;   // 本次开灯时刻
static unsigned long totalOnMs    = 0;   // 累计亮灯毫秒

#define HISTORY_SIZE 12
float   tempHistory[HISTORY_SIZE];
float   humiHistory[HISTORY_SIZE];
uint8_t historyIdx  = 0;
bool    historyFull = false;

void ledBegin() {
  strip.begin(); strip.clear(); strip.show();
  memset(tempHistory, 0, sizeof(tempHistory));
  memset(humiHistory, 0, sizeof(humiHistory));
  randomSeed(analogRead(0));
}

void pushHistory(float t, float h) {
  tempHistory[historyIdx] = t;
  humiHistory[historyIdx] = h;
  historyIdx = (historyIdx + 1) % HISTORY_SIZE;
  if (historyIdx == 0) historyFull = true;
}

// ===================================================
//  耗电统计
// ===================================================

// 估算当前灯带电流(mA)
float estimateCurrentMA() {
  if (!lightOn) return 0;
  // 根据亮度和颜色估算平均电流
  float bScale = brightness / 100.0;
  uint8_t r = (rgbColor >> 16) & 0xFF;
  uint8_t g = (rgbColor >> 8)  & 0xFF;
  uint8_t b =  rgbColor        & 0xFF;
  float colorScale = (r + g + b) / (255.0 * 3);
  // 动态效果平均亮度约50%
  if (lightMode=="rainbow"||lightMode=="flow"||lightMode=="wave"||
      lightMode=="meteor" ||lightMode=="colorbreath")
    colorScale = 0.5;
  if (lightMode=="sparkle"||lightMode=="breath")  colorScale = 0.3;
  if (lightMode=="candle")                        colorScale = 0.4;
  if (lightMode=="white"||lightMode=="warm"||lightMode=="cool") colorScale = 0.9;
  float ledMA = LED_COUNT * LED_MA_FULL * bScale * colorScale;
  return ledMA + MCU_MA;
}

// 估算当前功率(W)
float estimatePowerW() {
  return estimateCurrentMA() / 1000.0 * LED_VOLTAGE;
}

// 累计亮灯时长(秒)
unsigned long getTotalOnSeconds() {
  unsigned long total = totalOnMs;
  if (lightOn && lightOnStart > 0)
    total += millis() - lightOnStart;
  return total / 1000UL;
}

// 累计耗电(Wh)，简化：用平均功率×时长
// 实际每次开关都记录，这里用运行时长×当前功率估算
float getTotalWh() {
  return estimatePowerW() * (getTotalOnSeconds() / 3600.0);
}

// 开关灯时更新计时
void setLightOn(bool on) {
  if (on && !lightOn) {
    lightOnStart = millis();
  } else if (!on && lightOn) {
    if (lightOnStart > 0)
      totalOnMs += millis() - lightOnStart;
    lightOnStart = 0;
    strip.clear(); strip.show();
  }
  lightOn = on;
}

// ===================================================
//  工具
// ===================================================
uint8_t toV(uint8_t b) { return (uint8_t)((b / 100.0) * 255); }

void setAll(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LED_COUNT; i++)
    strip.setPixelColor(i, strip.Color(r, g, b));
  strip.show();
}

// ===================================================
//  原有效果
// ===================================================
void effectWhite()  { uint8_t v=toV(brightness); setAll(v,v,v); }
void effectWarm()   { uint8_t v=toV(brightness); setAll(v,(uint8_t)(v*.55),(uint8_t)(v*.08)); }
void effectCool()   { uint8_t v=toV(brightness); setAll((uint8_t)(v*.72),(uint8_t)(v*.85),v); }

void effectCustom() {
  float s=brightness/100.0;
  setAll(((rgbColor>>16)&0xFF)*s,((rgbColor>>8)&0xFF)*s,(rgbColor&0xFF)*s);
}

void effectRainbow() {
  for (int i=0;i<LED_COUNT;i++) {
    uint16_t hue=(effectStep+i*(65536/LED_COUNT))&0xFFFF;
    strip.setPixelColor(i,strip.gamma32(strip.ColorHSV(hue,255,toV(brightness))));
  }
  strip.show(); effectStep+=400;
}

void effectBreath() {
  float s=(breathVal/255.0)*(brightness/100.0);
  setAll(((rgbColor>>16)&0xFF)*s,((rgbColor>>8)&0xFF)*s,(rgbColor&0xFF)*s);
  if(breathUp){breathVal+=3;if(breathVal>=252)breathUp=false;}
  else        {breathVal-=3;if(breathVal<=3)  breathUp=true; }
}

void effectFlow() {
  strip.clear();
  int pos=effectStep%LED_COUNT;
  for(int i=0;i<8;i++){
    int idx=(pos+i)%LED_COUNT;
    uint16_t hue=(effectStep*300+i*6000)&0xFFFF;
    strip.setPixelColor(idx,strip.gamma32(strip.ColorHSV(hue,255,toV(brightness))));
  }
  strip.show(); effectStep++;
}

void effectAmbient(float temperature) {
  uint8_t v=toV(brightness);
  if      (temperature>=28) setAll(v,(uint8_t)(v*.25),0);
  else if (temperature<=16) setAll(0,(uint8_t)(v*.25),v);
  else                      setAll((uint8_t)(v*.15),(uint8_t)(v*.65),(uint8_t)(v*.2));
}

// ===================================================
//  新增效果
// ===================================================

// 流星雨
void effectMeteor() {
  strip.clear();
  int pos=effectStep%(LED_COUNT+12);
  for(int i=0;i<12;i++){
    int idx=pos-i;
    if(idx>=0&&idx<LED_COUNT){
      uint8_t fade=toV(brightness)*(12-i)/12;
      strip.setPixelColor(idx,strip.Color(fade,fade,(uint8_t)(fade*.5)));
    }
  }
  strip.show(); effectStep++;
}

// 星光闪烁
void effectSparkle() {
  for(int i=0;i<4;i++) strip.setPixelColor(random(LED_COUNT),0);
  for(int i=0;i<2;i++){
    uint8_t v=toV(brightness);
    strip.setPixelColor(random(LED_COUNT),strip.Color(v,v,v));
  }
  strip.show();
}

// 彩色呼吸
void effectColorBreath() {
  uint16_t hue=(effectStep*50)&0xFFFF;
  float s=(breathVal/255.0)*(brightness/100.0);
  uint32_t c=strip.gamma32(strip.ColorHSV(hue,255,255));
  setAll(((c>>16)&0xFF)*s,((c>>8)&0xFF)*s,(c&0xFF)*s);
  if(breathUp){breathVal+=2;if(breathVal>=252){breathUp=false;effectStep+=300;}}
  else        {breathVal-=2;if(breathVal<=3)   breathUp=true;}
}

// 警报闪烁
void effectAlert() {
  uint8_t v=toV(brightness);
  if((effectStep/5)%2==0) setAll(v,0,0);
  else                    setAll(0,0,v);
  effectStep++;
}

// 烛光
void effectCandle() {
  uint8_t base=toV(brightness);
  for(int i=0;i<LED_COUNT;i++){
    uint8_t f=random(base>50?base-50:0,base+1);
    strip.setPixelColor(i,strip.Color(f,(uint8_t)(f*.3),(uint8_t)(f*.02)));
  }
  strip.show();
}

// 彩虹波浪
void effectWave() {
  for(int i=0;i<LED_COUNT;i++){
    uint16_t hue=(effectStep*200+i*(65536/LED_COUNT))&0xFFFF;
    float sinV=(sin((i+effectStep*.1)*.4)+1.0)*.5;
    uint8_t v=(uint8_t)(toV(brightness)*sinV);
    strip.setPixelColor(i,strip.gamma32(strip.ColorHSV(hue,255,v)));
  }
  strip.show(); effectStep++;
}

// 警察灯：红蓝分段交替
void effectPolice() {
  strip.clear();
  int half=LED_COUNT/2;
  uint8_t v=toV(brightness);
  bool phase=(effectStep/8)%2;
  for(int i=0;i<half;i++)
    strip.setPixelColor(i, phase?strip.Color(v,0,0):0);
  for(int i=half;i<LED_COUNT;i++)
    strip.setPixelColor(i,!phase?strip.Color(0,0,v):0);
  strip.show(); effectStep++;
}

// 彩色分段：把灯带分成几段不同颜色
void effectSegment() {
  int seg=LED_COUNT/5;
  uint8_t v=toV(brightness);
  uint32_t colors[]={
    strip.Color(v,0,0),
    strip.Color(v,(uint8_t)(v*.5),0),
    strip.Color(0,v,0),
    strip.Color(0,0,v),
    strip.Color((uint8_t)(v*.5),0,v)
  };
  for(int i=0;i<LED_COUNT;i++)
    strip.setPixelColor(i,colors[(i/seg)%5]);
  strip.show();
}

// 跑马灯：单色追逐
void effectChase() {
  strip.clear();
  uint8_t v=toV(brightness);
  uint8_t r=(rgbColor>>16)&0xFF, g=(rgbColor>>8)&0xFF, b=rgbColor&0xFF;
  float s=brightness/100.0;
  for(int i=0;i<LED_COUNT;i+=3){
    int idx=(i+effectStep)%LED_COUNT;
    strip.setPixelColor(idx,strip.Color(r*s,g*s,b*s));
  }
  strip.show(); effectStep++;
}

// 火焰效果：红橙黄随机跳动
void effectFire() {
  for(int i=0;i<LED_COUNT;i++){
    uint8_t heat=random(100,255);
    float s=brightness/100.0;
    uint8_t r=(uint8_t)(heat*s);
    uint8_t g=(uint8_t)(heat*.3*s);
    uint8_t b=0;
    strip.setPixelColor(i,strip.Color(r,g,b));
  }
  strip.show();
}

// 海洋：蓝绿渐变波动
void effectOcean() {
  for(int i=0;i<LED_COUNT;i++){
    float wave=(sin((i+effectStep)*.3)+1.0)*.5;
    uint8_t v=(uint8_t)(toV(brightness)*wave);
    strip.setPixelColor(i,strip.Color(0,(uint8_t)(v*.4),v));
  }
  strip.show(); effectStep++;
}

// 彩虹分段静止
void effectRainbowStatic() {
  for(int i=0;i<LED_COUNT;i++){
    uint16_t hue=(uint32_t)i*65536/LED_COUNT;
    strip.setPixelColor(i,strip.gamma32(strip.ColorHSV(hue,255,toV(brightness))));
  }
  strip.show();
}

// ===================================================
//  统一调度
// ===================================================
void updateLED(float temperature) {
  if (!lightOn) return;
  unsigned long now=millis();
  bool dyn =(lightMode=="rainbow"||lightMode=="breath"||lightMode=="flow"||
             lightMode=="meteor" ||lightMode=="colorbreath"||lightMode=="alert"||
             lightMode=="wave"   ||lightMode=="police"||lightMode=="chase"||
             lightMode=="ocean");
  bool slow=(lightMode=="candle"||lightMode=="sparkle"||lightMode=="fire");
  bool stat=(lightMode=="white"||lightMode=="warm"||lightMode=="cool"||
             lightMode=="custom"||lightMode=="segment"||lightMode=="rainbowstatic"||
             lightMode=="ambient");
  if (dyn  && now-lastEffect<30)  return;
  if (slow && now-lastEffect<80)  return;
  if (stat && now-lastEffect<200) return;
  lastEffect=now;

  if      (lightMode=="white")         effectWhite();
  else if (lightMode=="warm")          effectWarm();
  else if (lightMode=="cool")          effectCool();
  else if (lightMode=="custom")        effectCustom();
  else if (lightMode=="rainbow")       effectRainbow();
  else if (lightMode=="breath")        effectBreath();
  else if (lightMode=="flow")          effectFlow();
  else if (lightMode=="ambient")       effectAmbient(temperature);
  else if (lightMode=="meteor")        effectMeteor();
  else if (lightMode=="sparkle")       effectSparkle();
  else if (lightMode=="colorbreath")   effectColorBreath();
  else if (lightMode=="alert")         effectAlert();
  else if (lightMode=="candle")        effectCandle();
  else if (lightMode=="wave")          effectWave();
  else if (lightMode=="police")        effectPolice();
  else if (lightMode=="segment")       effectSegment();
  else if (lightMode=="chase")         effectChase();
  else if (lightMode=="fire")          effectFire();
  else if (lightMode=="ocean")         effectOcean();
  else if (lightMode=="rainbowstatic") effectRainbowStatic();
  else effectWhite();
}
