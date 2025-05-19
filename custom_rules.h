// custom_rules.h
#ifndef CUSTOM_RULES_H
#define CUSTOM_RULES_H

#include "config.h"
#include <functional>

// Timestamps para controlar IH/IL
extern time_t ruleHighDT;
extern time_t ruleLowDT;

// Verifica regras avançadas (customSchedule) em cfg.
// Sempre que encontra uma regra cuja ação difere do estado atual do pino,
// chama onAction(relayVal, durationSec) e retorna o prefixo do evento (e.g. "DH12:00:00").
// Se nada disparar, retorna "".
String checkCustomRules(const Config& cfg,
                        int pin,
                        std::function<void(bool relayVal, unsigned long durationSec)> onAction);

#endif // CUSTOM_RULES_H
