// schedule.h
#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "config.h"
#include <functional>

// Retorna uma string descrevendo quanto falta para o próximo agendamento.
// Exemplo: "Próxima em: 01:23:45 (duração 00:05:00)"
String getNextTriggerTimeString(const Config& cfg);

// Verifica todos os agendamentos em cfg.schedules.
// Para cada slot cujo horário coincidir exatamente com o tempo atual e que ainda não tenha disparado hoje,
// chama onTrigger(cfg.schedules[i].durationSec), registra o log e salva cfg.
// Respeita o cooldown cfg.FEED_COOLDOWN.
// Se cfg.customEnabled == true, nada é feito.
void checkSchedules(Config& cfg,
                    std::function<void(unsigned long durationSec)> onTrigger);

#endif // SCHEDULE_H
