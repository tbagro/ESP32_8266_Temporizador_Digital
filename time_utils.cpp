// time_utils.cpp

#include "time_utils.h"
#include <RTClib.h>
#include <TimeLib.h>


// rtc e flag definidas em main ou em outro módulo
extern RTC_DS3231 rtc;
extern bool rtcInitialized;

static constexpr int YEAR_MIN = 2000;

String formatHHMMSS(int secs) {
  char buf[9];
  formatHHMMSS(secs, buf, sizeof(buf));
  return String(buf);
}

void formatHHMMSS(int secs, char* buf, size_t bufSize) {
  secs = abs(secs);
  int h = secs / 3600;
  int m = (secs % 3600) / 60;
  int s = secs % 60;
  snprintf(buf, bufSize, "%02d:%02d:%02d", h, m, s);
}

int parseHHMMSS(const String& s) {
  int h = 0, m = 0, sec = 0;
  if (sscanf(s.c_str(), "%d:%d:%d", &h, &m, &sec) == 3) {
    if (h >= 0 && h < 24 && m >= 0 && m < 60 && sec >= 0 && sec < 60) {
      return h * 3600 + m * 60 + sec;
    }
  }
  return -1;
}

int getCurrentTimeInSec() {
  return hour() * 3600 + minute() * 60 + second();
}

int calculateDayOfYear(int y, int m, int d) {
  static const int mdays[] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
  int days = 0;
  bool leap = ( (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0) );
  for (int i = 1; i < m; ++i) {
    days += (i == 2 && leap) ? 29 : mdays[i];
  }
  days += d;
  return days;
}

int getCurrentDayOfYear() {
  time_t t = now();
  return calculateDayOfYear(year(t), month(t), day(t));
}

String timeStr(const time_t &t) {
  char buf[9];
  int h = hour(t), m = minute(t), s = second(t);
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
  return String(buf);
}

String hhmmStr(const time_t &t) {
  char buf[6];
  int h = hour(t), m = minute(t);
  snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
  return String(buf);
}

time_t parseDateTime(const String& s) {
  struct tm tm_struct = {0};
  // formata "YYYY-MM-DD HH:MM"
  if (sscanf(s.c_str(), "%4d-%2d-%2d %2d:%2d",
             &tm_struct.tm_year,
             &tm_struct.tm_mon,
             &tm_struct.tm_mday,
             &tm_struct.tm_hour,
             &tm_struct.tm_min) == 5) {
    tm_struct.tm_year -= 1900;
    tm_struct.tm_mon  -= 1;
    tm_struct.tm_sec   = 0;
    tm_struct.tm_isdst = -1;
    return mktime(&tm_struct);
  }
  return (time_t)0;
}

void syncTimeLibWithRTC() {
  if (!rtcInitialized) return;
  DateTime dt = rtc.now();
  int yr = dt.year();
  time_t nowUnix = time(nullptr);
  int curYear = year(nowUnix);
  // valida ano
  if (yr < YEAR_MIN || yr > curYear + 1) {
    Serial.printf("RTC data inválida: %04d-%02d-%02d\n", yr, dt.month(), dt.day());
    return;
  }
  setTime(dt.unixtime());
}

String getCurrentDateTimeString() {
  time_t t = now();
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           year(t), month(t), day(t),
           hour(t), minute(t), second(t));
  return String(buf);
}
