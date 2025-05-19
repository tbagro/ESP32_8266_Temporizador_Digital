// custom_rules.cpp

#include "custom_rules.h"
#include "time_utils.h"
#include <TimeLib.h>

extern String   eventLog;
extern bool     isOutputActive;
extern time_t   ruleHighDT;
extern time_t   ruleLowDT;

// Obtém número de segundos de IH/IL dentro da string de regras
static int getRuleTime(const String& rules, const String& prefix) {
  int pos = rules.indexOf(prefix);
  if (pos < 0) return -1;
  String t = rules.substring(pos + prefix.length(), pos + prefix.length() + 8);
  return parseHHMMSS(t);
}

String checkCustomRules(const Config& cfg,
                        int pin,
                        std::function<void(bool relayVal, unsigned long durationSec)> onAction) {
  if (!cfg.customEnabled) return "";
  if (!onAction)          return "";

  time_t nowT = now();
  static time_t lastCheck = 0;
  if (nowT == lastCheck) return "";
  lastCheck = nowT;

  // monta "YYYY-MM-DD HH:MM"
  char dtBuf[17];
  snprintf(dtBuf, sizeof(dtBuf), "%04d-%02d-%02d %02d:%02d",
           year(nowT), month(nowT), day(nowT),
           hour(nowT), minute(nowT));
  String dtStr(dtBuf);

  String rules = cfg.customSchedule;
  String event;
  bool desiredState = false;

  // 1) Specific SH / SL
  if      (rules.indexOf("SH" + dtStr) != -1) { event = "SH" + dtStr; desiredState = true; }
  else if (rules.indexOf("SL" + dtStr) != -1) { event = "SL" + dtStr; desiredState = false; }
  else {
    // 2) Diário DH / DL (HH:MM:SS)
    int nowSec = getCurrentTimeInSec();
    char hmsBuf[9];
    formatHHMMSS(nowSec, hmsBuf, sizeof(hmsBuf));
    String hmsStr(hmsBuf);

    if      (rules.indexOf("DH" + hmsStr) != -1) { event = "DH" + hmsStr; desiredState = true; }
    else if (rules.indexOf("DL" + hmsStr) != -1) { event = "DL" + hmsStr; desiredState = false; }
    else {
      // 3) Semanal WH / WL (d HH:MM:SS)
      String dow = String(weekday(nowT)) + " " + hmsStr;
      if      (rules.indexOf("WH" + dow) != -1) { event = "WH" + dow; desiredState = true; }
      else if (rules.indexOf("WL" + dow) != -1) { event = "WL" + dow; desiredState = false; }
      else {
        int pinState = digitalRead(pin);
        // 4) Intervalo IH: se HIGH há >= IH segundos
        if (pinState == HIGH && ruleHighDT != 0) {
          int ih = getRuleTime(rules, "IH");
          if (ih >= 0 && (nowT - ruleHighDT) >= ih) {
            char buf[9]; formatHHMMSS(ih, buf, sizeof(buf));
            event = "IH" + String(buf);
            desiredState = false;
          }
        }
        // 5) Intervalo IL: se LOW há >= IL segundos
        if (event == "" && pinState == LOW && ruleLowDT != 0) {
          int il = getRuleTime(rules, "IL");
          if (il >= 0 && (nowT - ruleLowDT) >= il) {
            char buf[9]; formatHHMMSS(il, buf, sizeof(buf));
            event = "IL" + String(buf);
            desiredState = true;
          }
        }
      }
    }
  }

  // se alguma regra disparou **e** a ação difere do estado atual do pino
  int current = digitalRead(pin);
  if (event.length() > 0 && ((desiredState && current == LOW) || (!desiredState && current == HIGH))) {
    // log
    String log = timeStr(nowT)
               + " -> Regra " + event
               + " detectada para " + (desiredState ? "LIGAR" : "DESLIGAR") + " saída.";
    Serial.println(log);
    eventLog += log + "\n";

    // executa ação: HIGH -> startOutput(infinito), LOW -> stopOutput()
    if (desiredState) {
      onAction(true, (unsigned long)MAX_FEED_DURATION + 1);
      ruleHighDT = nowT;
      ruleLowDT  = 0;
    } else {
      onAction(false, 0);
      ruleLowDT  = nowT;
      ruleHighDT = 0;
    }

    return event;
  }

  return "";
}
