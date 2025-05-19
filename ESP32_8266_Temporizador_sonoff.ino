
// main.cpp

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif

#include <WiFiManager.h>
#include <time.h>

#include "config.h"
#include "time_utils.h"
#include "schedule.h"
#include "custom_rules.h"
#include "webserver.h"

// ===== Defaults por plataforma =====
#if defined(SONOFF_BASIC)
static constexpr int defaultFeederPin = 12;
#elif defined(ESP8266)
static constexpr int defaultFeederPin = 14;  // D5 no NodeMCU
#else
static constexpr int defaultFeederPin = 5;
#endif

#if defined(SONOFF_BASIC)
static constexpr int STATUS_LED_PIN = 13;
static constexpr int BUTTON_PIN     = 0;
#elif defined(ESP8266)
static constexpr int STATUS_LED_PIN = LED_BUILTIN;
static constexpr int BUTTON_PIN     = -1;
#else
static constexpr int STATUS_LED_PIN = 2;
static constexpr int BUTTON_PIN     = -1;
#endif

static constexpr char NTP_SERVER[] = "time.google.com";

// ===== Estado Global =====
Config           cfg;
WebSrv           server(80);
RTC_DS3231       rtc;
bool             rtcInitialized   = false;
unsigned long    lastTriggerMs    = 0;
String           eventLog         = "";
bool             isOutputActive   = false;
time_t           ruleHighDT       = 0;
time_t           ruleLowDT        = 0;
WiFiManager      wifiManager;

// Prototipos de funções auxiliares
void setupHardware();
void setupNetwork();
void updateStatusLED();
void startOutput(unsigned long durationSec);
void stopOutput();

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== Iniciando Temporizador Inteligente ===");

  // 1) Valores default antes de tentar carregar
  cfg.feederPin          = defaultFeederPin;
  cfg.manualDurationSec  = 5;
  cfg.customSchedule[0]  = '\0';
  cfg.customEnabled      = false;
  cfg.scheduleCount      = 0;

  // 2) Load / Save config
  if (loadConfig(cfg)) {
    Serial.println("Configurações carregadas do FS.");
  } else {
    Serial.println("Usando configurações padrão e criando arquivo.");
    saveConfig(cfg);
  }

  // 3) GPIOs
  setupHardware();

  // 4) RTC DS3231
  Wire.begin();
  if (rtc.begin()) {
    rtcInitialized = true;
    Serial.println("RTC DS3231 detectado.");
  } else {
    Serial.println("RTC DS3231 não encontrado.");
  }

  // 5) Rede e NTP/RTC Sync
  setupNetwork();

  // 6) HTTP server
  initWebServer(server, cfg);
}

void loop() {
  server.handleClient();
  updateStatusLED();

  // sincronização periódica RTC -> TimeLib (a cada 5min)
  static unsigned long lastSync = 0;
  if (rtcInitialized && millis() - lastSync >= 300000UL) {
    lastSync = millis();
    syncTimeLibWithRTC();
  }

  // processa regras customizadas ou schedules
  if (cfg.customEnabled) {
    checkCustomRules(cfg, cfg.feederPin,
      [&](bool relayVal, unsigned long dur){
        if (relayVal) startOutput(dur);
        else          stopOutput();
      }
    );
  } else {
    checkSchedules(cfg,
      [&](unsigned long dur){
        startOutput(dur);
      }
    );
  }

  // evita bloqueio excessivo
  delay(1);
}

// ===== Implementações Auxiliares =====

void setupHardware() {
  pinMode(cfg.feederPin, OUTPUT);
  digitalWrite(cfg.feederPin, LOW);

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, HIGH);

  #ifdef SONOFF_BASIC
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  #endif
}

void updateStatusLED() {
  static unsigned long prev = 0;
  static bool state = false;
  unsigned long nowMs = millis();

  if (isOutputActive) {
    // pisca rápido
    if (nowMs - prev >= 200) {
      state = !state;
      digitalWrite(STATUS_LED_PIN, state ? LOW : HIGH);
      prev = nowMs;
    }
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(STATUS_LED_PIN, LOW);
    } else {
      // pisca devagar se sem Wi-Fi
      if (nowMs - prev >= 1000) {
        state = !state;
        digitalWrite(STATUS_LED_PIN, state ? LOW : HIGH);
        prev = nowMs;
      }
    }
  }
}

void setupNetwork() {
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setConnectTimeout(30);

  if (!wifiManager.autoConnect("TemporizadorAP")) {
    Serial.println("Falha no Wi-Fi e portal expirou. Reiniciando...");
    delay(3000);
    ESP.restart();
  }
  Serial.print("Wi-Fi OK, IP: ");
  Serial.println(WiFi.localIP());

  // configura NTP (UTC) e sincroniza
  configTime(0, 0, NTP_SERVER);
  Serial.println("Aguardando NTP...");
  time_t t = 0;
  int tries = 0;
  while (tries++ < 30 && (t = time(nullptr)) < 1600000000UL) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (t < 1600000000UL) {
    Serial.println("NTP falhou.");
    if (rtcInitialized && !rtc.lostPower()) {
      Serial.println("Usando RTC como fallback.");
      syncTimeLibWithRTC();
    }
  } else {
    // aplica fuso e salva em TimeLib
    time_t local = t + GMT_OFFSET_SEC + DAYLIGHT_OFFSET_SEC;
    setTime(local);
    Serial.print("Hora NTP aplicada: ");
    Serial.println(getCurrentDateTimeString());

    if (rtcInitialized) {
      rtc.adjust(DateTime(year(local),
                         month(local),
                         day(local),
                         hour(local),
                         minute(local),
                         second(local)));
      Serial.println("RTC ajustado com NTP.");
    }
  }
}

void startOutput(unsigned long durationSec) {
  if (isOutputActive) return;
  unsigned long nowMs = millis();
  if (lastTriggerMs && nowMs - lastTriggerMs < (unsigned long)FEED_COOLDOWN * 1000UL) return;

  digitalWrite(cfg.feederPin, HIGH);
  isOutputActive   = true;
  lastTriggerMs    = nowMs;
  eventLog        += timeStr(now()) + " -> Saída LIGADA\n";
}

void stopOutput() {
  if (!isOutputActive) return;
  digitalWrite(cfg.feederPin, LOW);
  isOutputActive = false;
  eventLog      += timeStr(now()) + " -> Saída DESLIGADA\n";
}


