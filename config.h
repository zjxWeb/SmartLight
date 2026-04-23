#pragma once

// ===== WiFi =====
#define WIFI_SSID     "wifi名称"
#define WIFI_PASS     "wifi密码"

// ===== 巴法云 MQTT =====
// 连接方式：ClientID = 用户私钥，用户名密码留空
#define BEMFA_HOST    "bemfa.com"
#define BEMFA_PORT    9501
#define BEMFA_UID     "密钥"  // 控制台右上角获取

// 主题名（后三位决定设备类型）
// 002 = 灯泡设备，消息格式: on#亮度(0-100)#RGB十进制 / off
// 004 = 传感器设备，上报格式: #温度#湿度
#define TOPIC_LIGHT   "mylight002"
#define TOPIC_SENSOR  "mysensor004"
#define TOPIC_MODE    "mymode006"

// ===== 天气API (心知天气免费版) =====
#define WEATHER_KEY      "密钥"
#define WEATHER_CITY     "qingyang"
#define WEATHER_NOW_URL  "http://api.seniverse.com/v3/weather/now.json?key=" WEATHER_KEY "&location=" WEATHER_CITY "&language=zh-Hans&unit=c"
#define WEATHER_3D_URL   "http://api.seniverse.com/v3/weather/daily.json?key=" WEATHER_KEY "&location=" WEATHER_CITY "&language=zh-Hans&unit=c&start=0&days=3"

// ===== 耗电统计 =====
// WS2812B 每颗全亮约60mA，5V供电
#define LED_MA_FULL   60     // 每颗全亮电流(mA)
#define LED_VOLTAGE   5.0    // 供电电压(V)
// NodeMCU自身约80mA
#define MCU_MA        80
#define OTA_HOSTNAME  "SmartLight"
#define OTA_PASSWORD  "smartlight123"   // OTA升级密码，建议修改

// ===== OLED 切换间隔(毫秒) =====
#define OLED_INTERVAL 6000   // 每页显示6秒

// ===== NTP 时间 =====
#define NTP_SERVER    "ntp.aliyun.com"
#define NTP_OFFSET    28800  // UTC+8

// ===== 引脚 =====
#define PIN_DHT       13   // 改到 D7，避免占用GPIO2板载蓝灯
#define PIN_LED       14   // D5
#define LED_COUNT     30

// 板载LED引脚（低电平亮，高电平灭）
#define PIN_BUILTIN_LED1  2   // GPIO2 蓝灯
#define PIN_BUILTIN_LED2  16  // GPIO16 蓝灯

// ===== OLED I2C =====
#define OLED_ADDR     0x3C
#define OLED_WIDTH    128
#define OLED_HEIGHT   64
