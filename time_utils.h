
#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>   
#include <TimeLib.h>

// <— adicionado para ter String, snprintf, etc.
// formata segundos em "HH:MM:SS"

String formatHHMMSS(int secs);
void   formatHHMMSS(int secs, char* buf, size_t bufSize);

// parse "HH:MM:SS" para segundos
int parseHHMMSS(const String& s);

// retorna segundos desde meia-noite
int getCurrentTimeInSec();

// calcula dia do ano (1–366)
int calculateDayOfYear(int year, int month, int day);
int getCurrentDayOfYear();

// converte time_t para "HH:MM:SS"
String timeStr(const time_t &t);
// converte time_t para "HH:MM"
String hhmmStr(const time_t &t);

// parse "YYYY-MM-DD HH:MM" para time_t
time_t parseDateTime(const String& s);

// sincroniza TimeLib com RTC DS3231 (ignora datas inválidas)
void syncTimeLibWithRTC();

// RETORNA "YYYY-MM-DD HH:MM:SS"
String getCurrentDateTimeString();

#endif // TIME_UTILS_H