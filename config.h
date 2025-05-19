// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h>
#include <FS.h>

// ===== Constantes Globais =====
static constexpr char   CONFIG_PATH[]       = "/config.json";
static constexpr int    MAX_SLOTS           = 10;
static constexpr long   GMT_OFFSET_SEC      = -4L * 3600L;  // UTC–4
static constexpr int    DAYLIGHT_OFFSET_SEC = 0;            // Horário de verão
static constexpr int    FEED_COOLDOWN       = 10;           // s entre ativações
static constexpr int    MAX_FEED_DURATION   = 300;          // s (5 min)

// ===== Estruturas de Configuração =====
struct Schedule {
  int timeSec;        // segundos desde meia-noite
  int durationSec;    // duração da ativação em segundos
  int lastTriggerDay; // dia do ano do último acionamento
};

struct Config {
  int           feederPin;            // pino de saída
  unsigned long manualDurationSec;    // duração manual padrão (s)
  char          customSchedule[512];  // regras avançadas em texto
  bool          customEnabled;        // se regras avançadas estão ativas
  Schedule      schedules[MAX_SLOTS]; // lista de agendamentos
  int           scheduleCount;        // total de agendamentos válidos
};

// ===== Protótipos =====
bool loadConfig(Config& cfg);
bool saveConfig(const Config& cfg);

#endif // CONFIG_H
