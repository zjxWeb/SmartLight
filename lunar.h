#pragma once

static const uint32_t lunarInfo[] = {
  0x04AE53,0x0A5748,0x5526BD,0x0D2650,0x0D9544,0x46AAB9,0x056A4D,0x09AD42,0x24AEB6,0x04AE4A,
  0x6AA550,0x0B5544,0x4BBEAB,0x0AD550,0x056D45,0x4AEBB9,0x025D4D,0x092D42,0x2C95B6,0x0A954A,
  0x7B4AC5,0x06CA50,0x0B5543,0x4BDAB7,0x096D4B,0x14AEB3,0x0AD547,0x0B6A3C,0x6BAAB0,0x0AAE45,
  0x5B5AB5,0x056D4A,0x0A6D61,0x355B4E,0x025D45,0x0A9B3A,0x2A4BB7,0x0A4B4B,0x6AA4BE,0x0AD550,
  0x056D46,0x4AEBB9,0x025D4D,0x092D42,0x2C95B6,0x0A954A,0x7B4AC5,0x06CA50,0x0B5543,0x4BDAB7,
};

static const char* tianGan[]  = {"Jia","Yi","Bing","Ding","Wu","Ji","Geng","Xin","Ren","Gui"};
static const char* diZhi[]    = {"Zi","Chou","Yin","Mao","Chen","Si","Wu","Wei","Shen","You","Xu","Hai"};
static const char* shengXiao[]= {"Rat","Ox","Tiger","Rabbit","Dragon","Snake","Horse","Goat","Monkey","Rooster","Dog","Pig"};
static const char* lunarMonthName[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
static const char* lunarDayName[]   = {
  "","1st","2nd","3rd","4th","5th","6th","7th","8th","9th","10th",
  "11th","12th","13th","14th","15th","16th","17th","18th","19th","20th",
  "21st","22nd","23rd","24th","25th","26th","27th","28th","29th","30th"
};

struct LunarDate {
  int  year, month, day;
  bool isLeap;
  char ganZhi[12];
  char zodiac[12];
  char monthStr[8];
  char dayStr[8];
};

// ---- 前置声明，解决顺序依赖 ----
static int leapMonth(int y);
static int leapDays(int y);
static int monthDays(int y, int m);
static int lunarYearDays(int y);

static int leapMonth(int y) {
  return (int)(lunarInfo[y - 2000] & 0xf);
}

static int leapDays(int y) {
  if (leapMonth(y))
    return ((lunarInfo[y - 2000] & 0x10000) ? 30 : 29);
  return 0;
}

static int monthDays(int y, int m) {
  return ((lunarInfo[y - 2000] & (0x10000 >> m)) ? 30 : 29);
}

static int lunarYearDays(int y) {
  int i, sum = 348;
  for (i = 0x8000; i > 0x8; i >>= 1)
    sum += ((lunarInfo[y - 2000] & i) ? 1 : 0);
  return sum + leapDays(y);
}

LunarDate solarToLunar(int sYear, int sMonth, int sDay) {
  LunarDate result;
  const int baseYear = 2000, baseDay = 6; // 基准：2000-01-06 = 农历1999-12-01

  int offset = 0;
  for (int y = baseYear; y < sYear; y++)
    offset += ((y%4==0&&y%100!=0)||(y%400==0)) ? 366 : 365;

  int mdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
  if ((sYear%4==0&&sYear%100!=0)||(sYear%400==0)) mdays[2] = 29;
  for (int m = 1; m < sMonth; m++) offset += mdays[m];
  offset += sDay - baseDay;

  int lYear = 1999, lMonth = 11, lDay = 1;
  bool lLeap = false;
  int lYearDays = lunarYearDays(lYear);

  while (offset > 0) {
    if (offset >= lYearDays) {
      offset -= lYearDays;
      lYear++;
      lYearDays = lunarYearDays(lYear);
      continue;
    }
    int lm = leapMonth(lYear);
    for (int m = 1; m <= 12 && offset > 0; m++) {
      int days;
      if (lm > 0 && m == lm + 1 && !lLeap) {
        m--; lLeap = true;
        days = leapDays(lYear);
      } else {
        lLeap = false;
        days = monthDays(lYear, m);
      }
      if (offset >= days) {
        offset -= days; lMonth = m;
      } else {
        lDay = offset + 1; offset = 0; lMonth = m; break;
      }
    }
  }

  result.year = lYear; result.month = lMonth;
  result.day  = lDay;  result.isLeap = lLeap;

  int ganIdx = (lYear - 4) % 10; if (ganIdx < 0) ganIdx += 10;
  int zhiIdx = (lYear - 4) % 12; if (zhiIdx < 0) zhiIdx += 12;
  snprintf(result.ganZhi,   sizeof(result.ganZhi),   "%s-%s", tianGan[ganIdx], diZhi[zhiIdx]);
  snprintf(result.zodiac,   sizeof(result.zodiac),   "%s",    shengXiao[zhiIdx]);
  snprintf(result.monthStr, sizeof(result.monthStr), "%s%s",  lLeap?"L.":"", lunarMonthName[lMonth]);
  snprintf(result.dayStr,   sizeof(result.dayStr),   "%s",    lunarDayName[lDay]);
  return result;
}
