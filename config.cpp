// config.cpp

#include "config.h"
#include <ArduinoJson.h>
#include <FS.h>
#include "time_utils.h"

#ifdef ESP8266
  #include <LittleFS.h>
  #define FS_INSTANCE LittleFS
#else
  #include <SPIFFS.h>
  #define FS_INSTANCE SPIFFS
#endif

bool loadConfig(Config& cfg) {
  // monta o sistema de arquivos (formatando se necessário)
#ifdef ESP8266
  if (!FS_INSTANCE.begin()) {
    Serial.println("Falha ao montar FS (ESP8266)");
    return false;  // ou false no saveConfig
  }
#else
  if (!FS_INSTANCE.begin(true)) {
    Serial.println("Falha ao montar FS (ESP32)");
    return false;
  }
#endif

  // se não há arquivo, retorna false (usar valores padrão)
  if (!FS_INSTANCE.exists(CONFIG_PATH)) {
    return false;
  }

  File file = FS_INSTANCE.open(CONFIG_PATH, "r");
  if (!file) {
    Serial.println("Não foi possível abrir config.json");
    return false;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) {
    Serial.print("Erro JSON em loadConfig: ");
    Serial.println(err.c_str());
    // descarta config corrompida
    FS_INSTANCE.remove(CONFIG_PATH);
    return false;
  }

  // carrega valores (ou mantém os que já estavam em cfg como default)
  cfg.feederPin         = doc["feederPin"]         | cfg.feederPin;
  cfg.manualDurationSec = doc["manualDuration"]    | cfg.manualDurationSec;
  if (doc.containsKey("customSchedule")) {
    const char* ptr = doc["customSchedule"];
    strncpy(cfg.customSchedule, ptr, sizeof(cfg.customSchedule) - 1);
    cfg.customSchedule[sizeof(cfg.customSchedule) - 1] = '\0';
  }
  cfg.customEnabled     = doc["customEnabled"]     | cfg.customEnabled;

  // schedules
  cfg.scheduleCount = 0;
  if (doc.containsKey("schedules")) {
    JsonArray arr = doc["schedules"].as<JsonArray>();
    for (JsonObject o : arr) {
      if (cfg.scheduleCount >= MAX_SLOTS) break;
      cfg.schedules[cfg.scheduleCount].timeSec        = o["time"]           | 0;
      cfg.schedules[cfg.scheduleCount].durationSec    = o["duration"]       | 0;
      cfg.schedules[cfg.scheduleCount].lastTriggerDay = o["lastTriggerDay"] | -1;
      cfg.scheduleCount++;
    }
  }

  Serial.printf("Config carregada: pin=%d, manualDur=%lus, regrasAtivas=%d, slots=%d\n",
                cfg.feederPin,
                cfg.manualDurationSec,
                cfg.customEnabled,
                cfg.scheduleCount);
  return true;
}

bool saveConfig(const Config& cfg) {
#ifdef ESP8266
  if (!FS_INSTANCE.begin()) {
    Serial.println("Falha ao montar FS (ESP8266)");
    return false;  // ou false no saveConfig
  }
#else
  if (!FS_INSTANCE.begin(true)) {
    Serial.println("Falha ao montar FS (ESP32)");
    return false;
  }
#endif

  DynamicJsonDocument doc(2048);
  doc["feederPin"]       = cfg.feederPin;
  doc["manualDuration"]  = cfg.manualDurationSec;
  doc["customSchedule"]  = cfg.customSchedule;
  doc["customEnabled"]   = cfg.customEnabled;

  JsonArray arr = doc.createNestedArray("schedules");
  for (int i = 0; i < cfg.scheduleCount && i < MAX_SLOTS; i++) {
    JsonObject o = arr.createNestedObject();
    o["time"]           = cfg.schedules[i].timeSec;
    o["duration"]       = cfg.schedules[i].durationSec;
    o["lastTriggerDay"] = cfg.schedules[i].lastTriggerDay;
  }

  File file = FS_INSTANCE.open(CONFIG_PATH, "w");
  if (!file) {
    Serial.println("Não foi possível abrir config.json para escrita");
    return false;
  }
  if (serializeJson(doc, file) == 0) {
    Serial.println("Falha ao serializar JSON em saveConfig");
    file.close();
    return false;
  }
  file.close();

  Serial.println("Configuração salva com sucesso");
  return true;
}
