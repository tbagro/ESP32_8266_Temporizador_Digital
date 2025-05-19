// schedule.cpp
#include "schedule.h"
#include "time_utils.h"
#include <TimeLib.h>

// Estes symbols devem estar definidos em outro módulo (por exemplo, main.cpp)
extern unsigned long lastTriggerMs;
extern String         eventLog;
bool saveConfig(const Config& cfg);

String getNextTriggerTimeString(const Config& cfg) {
  if (cfg.customEnabled) {
    return "Regras personalizadas ativas.";
  }
  if (cfg.scheduleCount == 0) {
    return "Nenhum agendamento configurado.";
  }

  int   nowSec    = getCurrentTimeInSec();
  int   secondsInDay = 24 * 3600;
  long  bestDiff  = secondsInDay + 1;
  int   bestDur   = 0;

  for (int i = 0; i < cfg.scheduleCount; i++) {
    const auto& s = cfg.schedules[i];
    // se já disparou hoje ou horário igual ao atual mas já disparou, pula
    int today = getCurrentDayOfYear();
    if (s.lastTriggerDay == today && s.timeSec == nowSec) continue;

    long diff;
    if (s.timeSec > nowSec) {
      diff = s.timeSec - nowSec;
    } else {
      diff = (secondsInDay - nowSec) + s.timeSec;
    }

    if (diff < bestDiff) {
      bestDiff = diff;
      bestDur  = s.durationSec;
    }
  }

  if (bestDiff <= secondsInDay) {
    String diffStr = formatHHMMSS((int)bestDiff);
    String durStr  = formatHHMMSS(bestDur);
    return "Próxima em: " + diffStr + " (duração " + durStr + ")";
  }

  return "Nenhum agendamento futuro encontrado.";
}

void checkSchedules(Config& cfg,
                    std::function<void(unsigned long)> onTrigger) {
  // não processa se custom rules ativas
  if (cfg.customEnabled) return;
  if (!onTrigger)        return;

  time_t nowT   = now();
  int today     = getCurrentDayOfYear();
  int nowSec    = getCurrentTimeInSec();
  unsigned long nowMs = millis();

  for (int i = 0; i < cfg.scheduleCount; i++) {
    auto& s = cfg.schedules[i];

    // horário exato e ainda não disparou hoje?
    if (s.timeSec == nowSec && s.lastTriggerDay != today) {
      // respeita cooldown
      if (lastTriggerMs == 0 || nowMs - lastTriggerMs >= (unsigned long)FEED_COOLDOWN * 1000UL) {
        // marca disparo
        s.lastTriggerDay = today;
        // log
        String log = timeStr(nowT)
                   + " -> Agendamento #" + String(i)
                   + " acionado (duração " + formatHHMMSS(s.durationSec) + ").";
        Serial.println(log);
        eventLog += log + "\n";

        // executa ação externa (por exemplo startOutput)
        onTrigger(s.durationSec);

        // persiste alterações
        saveConfig(cfg);

        // atualiza cooldown
        lastTriggerMs = nowMs;
      }
      else {
        String log = timeStr(nowT)
                   + " -> Agendamento #" + String(i)
                   + " ignorado (cooldown).";
        Serial.println(log);
        eventLog += log + "\n";
      }
    }
  }
}
