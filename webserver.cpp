

// webserver.cpp

#include "webserver.h"
#include "webpage.h"       // seu htmlPage em PROGMEM
#include "time_utils.h"
#include "schedule.h"
#include "custom_rules.h"
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif
#include <ArduinoJson.h>
#include <TimeLib.h>  

// Variáveis e funções definidas em main.cpp
extern Config          cfg;
extern WebSrv          server;
extern String          eventLog;
extern bool            isOutputActive;
extern unsigned long   lastTriggerMs;
extern void            startOutput(unsigned long durationSec);
extern void            stopOutput();

static bool isValidGpio(int pin) {
#ifdef ESP8266
  return pin >= 0 && pin <= 16 && pin != 1 && pin != 3;
#else
  return pin >= 0 && pin <= 33;
#endif
}

void initWebServer(WebSrv& server, Config& cfg) {
  // ---- Página raiz ----
  server.on("/", HTTP_GET, [&]() {
    String page = FPSTR(htmlPage);

    // Wi-Fi quality
    int rssi = WiFi.RSSI();
    int qual = map(constrain(rssi, -90, -30), -90, -30, 0, 100);
    page.replace("%WIFI_QUALITY%", String(qual));

    // Duração manual e pino de saída
    page.replace("%MANUAL%", formatHHMMSS(cfg.manualDurationSec));
    page.replace("%OUTPUT_PIN_VALUE%", String(cfg.feederPin));

    // Agendamentos
    String schedStr;
    for (int i = 0; i < cfg.scheduleCount; i++) {
      if (i) schedStr += ",";
      schedStr += formatHHMMSS(cfg.schedules[i].timeSec);
      schedStr += "|";
      schedStr += formatHHMMSS(cfg.schedules[i].durationSec);
    }
    page.replace("%SCHEDULES%", schedStr);

    // Regras customizadas
    page.replace("%CUSTOM_RULES%", String(cfg.customSchedule));
    page.replace("%TOGGLE_BUTTON%", cfg.customEnabled ? "Desativar Regras" : "Ativar Regras");

    // Classe de status do LED
    page.replace("%STATUS_CLASS%",
                 isOutputActive ? "feeding"
                                : (WiFi.status() == WL_CONNECTED ? "active" : ""));

    server.send(200, "text/html", page);
  });

  // ---- RSSI / Wi-Fi Quality ----
  server.on("/rssi", HTTP_GET, [&]() {
    int rssi = WiFi.RSSI();
    int pct  = map(constrain(rssi, -90, -30), -90, -30, 0, 100);
    String json = String("{\"rssi\":") + rssi + ",\"pct\":" + pct + "}";
    server.send(200, "application/json", json);
  });

  // ---- Hora atual ----
  server.on("/time", HTTP_GET, [&]() {
    server.send(200, "text/plain", timeStr(now()));
  });

  // ---- Próximo acionamento ----
  server.on("/nextTriggerTime", HTTP_GET, [&]() {
    server.send(200, "text/plain", getNextTriggerTimeString(cfg));
  });



  // ---- Alterar pino de saída ----
  server.on("/setFeederPin", HTTP_POST, [&]() {
    if (!server.hasArg("feederPin")) {
      server.send(400, "text/plain", "Parâmetro 'feederPin' ausente");
      return;
    }
    int newPin = server.arg("feederPin").toInt();
    if (!isValidGpio(newPin)) {
      server.send(400, "text/plain", "Pino inválido ou reservado");
      return;
    }
    // se mudar, desliga saída atual e reconfigura pino
    if (cfg.feederPin != newPin) {
      if (isOutputActive) {
        stopOutput();
        eventLog += timeStr(now()) + " -> Saída desligada para troca de pino\n";
      }
      cfg.feederPin = newPin;
      pinMode(cfg.feederPin, OUTPUT);
      digitalWrite(cfg.feederPin, LOW);
    }
    saveConfig(cfg);
    eventLog += timeStr(now()) + " -> Pino alterado para GPIO " + String(cfg.feederPin) + "\n";
    server.send(200, "text/plain", "Pino salvo");
  });

  // ---- Status atual ----
server.on("/status", HTTP_GET, [&]() {
  DynamicJsonDocument doc(256);
  doc["is_feeding"] = isOutputActive;
  doc["custom_rules_enabled"] = cfg.customEnabled;

  String activeRule   = "none";
  long   timeRemaining = -1;

  if (cfg.customEnabled) {
    time_t nowT = now();               // <-- agore sim existe!
    String rules = String(cfg.customSchedule);
    int ih = -1, il = -1;

    if (isOutputActive) {
      int p = rules.indexOf("IH");
      if (p >= 0) ih = parseHHMMSS(rules.substring(p+2, p+10));
      if (ih > 0) {
        activeRule   = "IH";
        timeRemaining = ih - (nowT - ruleHighDT);
      }
    } else {
      int p = rules.indexOf("IL");
      if (p >= 0) il = parseHHMMSS(rules.substring(p+2, p+10));
      if (il > 0) {
        activeRule   = "IL";
        timeRemaining = il - (nowT - ruleLowDT);
      }
    }
    if (timeRemaining < 0) timeRemaining = 0;
  }

  doc["active_custom_rule"]         = activeRule;
  doc["custom_rule_time_remaining"] = timeRemaining;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
});

  // ---- Ativação manual ----
  server.on("/feedNow", HTTP_POST, [&]() {
    unsigned long nowMs = millis();
    if (isOutputActive) {
      server.send(409, "text/plain", "Saída já ativa");
      return;
    }
    if (lastTriggerMs && nowMs - lastTriggerMs < (unsigned long)FEED_COOLDOWN * 1000UL) {
      server.send(429, "text/plain", "Aguarde intervalo entre ativações");
      return;
    }
    eventLog += timeStr(now()) + " -> FeedNow manual acionado\n";
    startOutput(cfg.manualDurationSec);
    server.send(200, "text/plain", "Saída ativada");
  });

  // ---- Desativar manual ----
  server.on("/stopFeedNow", HTTP_POST, [&]() {
    if (!isOutputActive) {
      server.send(400, "text/plain", "Nenhuma saída ativa");
      return;
    }
    eventLog += timeStr(now()) + " -> StopFeedNow manual acionado\n";
    stopOutput();
    server.send(200, "text/plain", "Saída desativada");
  });

  // ---- Ajustar duração manual ----
  server.on("/setManualDuration", HTTP_POST, [&]() {
    if (!server.hasArg("manualDuration")) {
      server.send(400, "text/plain", "Parâmetro 'manualDuration' ausente");
      return;
    }
    int secs = parseHHMMSS(server.arg("manualDuration"));
    if (secs <= 0 || secs > MAX_FEED_DURATION) {
      server.send(400, "text/plain", "Duração inválida (1–" +
                  formatHHMMSS(MAX_FEED_DURATION) + ")");
      return;
    }
    cfg.manualDurationSec = secs;
    saveConfig(cfg);
    eventLog += timeStr(now()) + " -> Duração manual ajustada para " +
                formatHHMMSS(secs) + "\n";
    server.send(200, "text/plain", "Duração salva");
  });

  // ---- Salvar agendamentos ----
  server.on("/setSchedules", HTTP_POST, [&]() {
    if (!server.hasArg("schedules")) {
      server.send(400, "text/plain", "Parâmetro 'schedules' ausente");
      return;
    }
    String arg = server.arg("schedules");
    cfg.scheduleCount = 0;
    for (int i = 0; i < MAX_SLOTS && arg.length(); i++) {
      int comma = arg.indexOf(',');
      String tok = comma > 0 ? arg.substring(0, comma) : arg;
      int sep = tok.indexOf('|');
      if (sep > 0) {
        int t = parseHHMMSS(tok.substring(0, sep));
        int d = parseHHMMSS(tok.substring(sep + 1));
        if (t >= 0 && d > 0 && d <= MAX_FEED_DURATION) {
          cfg.schedules[cfg.scheduleCount++] = { t, d, -1 };
        }
      }
      if (comma < 0) break;
      arg = arg.substring(comma + 1);
    }
    saveConfig(cfg);
    eventLog += timeStr(now()) + " -> " +
                String(cfg.scheduleCount) + " agendamentos salvos\n";
    server.send(200, "text/plain", "Agendamentos salvos");
  });

  // ---- Regras customizadas ----
  server.on("/setCustomRules", HTTP_POST, [&]() {
    if (!server.hasArg("rules")) {
      server.send(400, "text/plain", "Parâmetro 'rules' ausente");
      return;
    }
    String r = server.arg("rules");
    if (r.length() >= sizeof(cfg.customSchedule)) {
      server.send(400, "text/plain", "Regras muito longas");
      return;
    }
    r.toCharArray(cfg.customSchedule, sizeof(cfg.customSchedule));
    saveConfig(cfg);
    eventLog += timeStr(now()) + " -> Regras customizadas salvas\n";
    server.send(200, "text/plain", "Regras salvas");
  });

  // ---- Alternar regras ----
  server.on("/toggleCustomRules", HTTP_POST, [&]() {
    cfg.customEnabled = !cfg.customEnabled;
    saveConfig(cfg);
    eventLog += timeStr(now()) + " -> CustomRules " +
                String(cfg.customEnabled ? "ativadas\n" : "desativadas\n");
    server.send(200, "text/plain",
                cfg.customEnabled ? "Regras ativadas" : "Regras desativadas");
  });

  // ---- Logs de eventos ----
  server.on("/events", HTTP_GET, [&]() {
    if (eventLog.length()) {
      server.send(200, "text/plain", eventLog);
      eventLog = "";
    } else {
      server.send(200, "text/plain", "");
    }
  });

  // ---- Not Found ----
  server.onNotFound([&]() {
    server.send(404, "text/plain", "Rota não encontrada");
  });

  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

// Fim de webserver.cpp

