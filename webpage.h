// webpage.h
#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Temporizador Inteligente</title>
  <link href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.1/css/bootstrap.min.css" rel="stylesheet">
  <style>
    body { font-family: system-ui, -apple-system, sans-serif; margin:0 auto; padding:20px; max-width:600px; background:#f5f5f5; }
    h1 { text-align:center; color:#333; margin-bottom:30px; }
    .card { background:#fff; padding:20px; margin-bottom:20px; border-radius:8px; box-shadow:0 2px 4px rgba(0,0,0,0.1); }
    
    label {
      display: block;
      margin-bottom: 8px;
      color: #555;
      font-weight: 500;
    }
    input[type="time"],
    input[type="text"],
    input[type="number"],
    textarea {
      width: 100%;
      padding: 10px;
      border: 1px solid #ddd;
      border-radius: 4px;
      margin-bottom: 10px;
      box-sizing: border-box;
      font-family: inherit;
    }
    button {
      width: 100%;
      padding: 12px;
      background: #2e7d32;
      color: #fff;
      border: none;
      border-radius: 4px;
      font-weight: 500;
      cursor: pointer;
      margin-bottom: 10px;
    }
    button:hover {
      background: #256427;
    }
    button.secondary {
      background: #666;
    }
    button.secondary:hover {
      background: #555;
    }
    button.warning {
      background: #d32f2f;
    }
    button.warning:hover {
      background: #c62828;
    }
    button:disabled {
      background: #ccc;
      cursor: not-allowed;
    }
    ul {
      list-style: none;
      padding: 0;
    }
    li {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 8px;
      background: #f8f8f8;
      border-radius: 4px;
      margin-bottom: 8px;
    }
    .status-indicator {
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
      margin-bottom: 20px;
    }
    .led {
      width: 15px;
      height: 15px;
      border-radius: 50%;
      background: #ccc;
    }
    .led.active { /* LED verde para WiFi conectado e dispositivo inativo */
      background: #4caf50;
      box-shadow: 0 0 10px #4caf50;
    }
    .led.feeding { /* LED laranja para dispositivo ativo (piscando) - 'feeding' é a classe CSS, mantida por simplicidade */
      background: #ff9800; /* Laranja para "ativo" */
      box-shadow: 0 0 10px #ff9800;
      animation: pulse 1s infinite;
    }
    @keyframes pulse {
      0%   { opacity: 1; }
      50%  { opacity: 0.5; }
      100% { opacity: 1; }
    }
    #currentTime,
    #wifiQuality {
      font-size: 1.2em;
      color: #666;
      margin-left: 8px;
    }
    #nextTrigger, #customRuleCountdown {
      text-align: center;
      margin: 10px 0;
      padding: 10px;
      background: #e8f5e9;
      border-radius: 4px;
      color: #2e7d32;
      font-size: 1.1em;
    }
     #customRuleCountdown {
        background: #fff3e0;
        color: #e65100;
        display: none;
     }
    #eventConsole {
      background: #111;
      color: #0f0;
      font-family: monospace;
      white-space: pre-wrap;
      padding: 10px;
      height: 120px;
      overflow-y: auto;
      border: 1px solid #444;
      border-radius: 4px;
      margin-top: 10px;
    }
    .message {
      margin-top: 10px;
      padding: 10px;
      border-radius: 4px;
      text-align: center;
      font-weight: bold;
      min-height: 1.5em;
    }
    .message.success {
      background: #e6f4ea;
      color: #388e3c;
      border: 1px solid #66bb6a;
    }
    .message.error {
      background: #feebeb;
      color: #c62828;
      border: 1px solid #ef5350;
    }
    details > summary {
      padding: 8px;
      background-color: #eee;
      border: 1px solid #ddd;
      border-radius: 4px;
      cursor: pointer;
      font-weight: 500;
      margin-top: 15px;
    }
    details > div {
      padding:10px;
      background-color:#f9f9f9;
      border:1px solid #eee;
      border-top: none;
      border-radius: 0 0 4px 4px;
      margin-bottom: 10px;
    }
    details ul { margin-left: 20px; list-style-type: disc; }
    details code { background-color: #e8e8e8; padding: 2px 4px; border-radius: 3px; }
    .pin-table { width: 100%; border-collapse: collapse; margin-top: 10px; }
    .pin-table th, .pin-table td { border: 1px solid #ddd; padding: 6px; text-align: left; }
    .pin-table th { background-color: #f2f2f2; 

  </style>
</head>
<body>
  <h1>Temporizador Inteligente</h1>

  <section class="status-indicator">
    <div class="led %STATUS_CLASS%"></div> <div id="currentTime">--:--:--</div>
    <div id="wifiQuality">Wi‑Fi: %WIFI_QUALITY%%</div>
  </section>

  <section class="card">
    <button id="manualActivateOutput">Ativar Saída Agora</button>
    <button id="manualDeactivateOutput" class="warning" style="display: none;">Desativar Saída</button>
    <div id="manualOutputMessage" class="message"></div>
  </section>

  <section class="card">
    <form id="manualDurationForm">
      <label for="manualInterval">Duração do Pulso/Ativação (HH:MM:SS)</label>
      <input type="time" id="manualInterval" step="1" value="%MANUAL%" />
      <button type="submit">Salvar Duração</button>
      <div id="manualDurationMessage" class="message"></div>
    </form>
  </section>

  <section class="card">
    <form id="outputPinForm">
      <label for="outputPinInput">Pino de Saída (GPIO):</label>
      <input type="number" id="outputPinInput" value="%OUTPUT_PIN_VALUE%" min="0" max="39">
      <button type="submit">Salvar Pino de Saída</button>
      <div id="outputPinMessage" class="message"></div>
    </form>
    <details>
        <summary>Ajuda: Pinos Wemos D1 Mini (ESP8266)</summary>
        <div>
            <p>Abaixo uma referência dos pinos do Wemos D1 Mini e seus GPIOs correspondentes. Insira o número GPIO no campo acima.</p>
            <table class="pin-table">
                <thead><tr><th>Pino (Silk)</th><th>GPIO</th><th>Observações</th></tr></thead>
                <tbody>
                    <tr><td>D0</td><td>16</td><td>LED integrado (em algumas versões), sem PWM/I2C/SPI, cuidado ao usar.</td></tr>
                    <tr><td>D1</td><td>5</td><td>SCL (I2C)</td></tr>
                    <tr><td>D2</td><td>4</td><td>SDA (I2C)</td></tr>
                    <tr><td>D3</td><td>0</td><td>Flash Mode (nível BAIXO no boot entra em modo flash).</td></tr>
                    <tr><td>D4</td><td>2</td><td>TXD1, LED integrado (azul).</td></tr>
                    <tr><td>D5</td><td>14</td><td>HSCLK (SPI)</td></tr>
                    <tr><td>D6</td><td>12</td><td>HMISO (SPI)</td></tr>
                    <tr><td>D7</td><td>13</td><td>HMOSI/RXD2 (SPI)</td></tr>
                    <tr><td>D8</td><td>15</td><td>HCS (SPI), deve estar em nível BAIXO no boot.</td></tr>
                    <tr><td>RX</td><td>3</td><td>RXD0</td></tr>
                    <tr><td>TX</td><td>1</td><td>TXD0 (Debug)</td></tr>
                </tbody>
            </table>
            <p><small><strong>Recomendação:</strong> Para saídas simples, D1, D2, D5, D6, D7 são geralmente seguros. Evite D0, D3, D4, D8, RX, TX a menos que saiba as implicações.</small></p>
            <p><small>Para placas ESP32, consulte o pinout específico da sua placa.</small></p>
        </div>
    </details>
  </section>

  <section class="card">
    <form id="scheduleForm">
      <label for="newScheduleTime">Novo Horário de Ativação (HH:MM:SS)</label>
      <input type="time" id="newScheduleTime" step="1" />
      <label for="newScheduleInterval">Duração da Ativação (HH:MM:SS)</label>
      <input type="time" id="newScheduleInterval" step="1" />
      <button type="button" id="addSchedule">+ Adicionar Agendamento</button>
    </form>
    <ul id="scheduleList"></ul>
    <button id="saveSchedules">Salvar Agendamentos</button>
    <div id="scheduleFormMessage" class="message"></div>
  </section>

  <section class="card">
    <label for="customRulesInput">Regras Avançadas (Editar):</label>
    <textarea id="customRulesInput" rows="4" placeholder="Ex: IH00:00:30 IL00:01:00">%CUSTOM_RULES%</textarea>
    <button id="saveRules">Salvar Regras</button>
    <button id="toggleRules" class="secondary">%TOGGLE_BUTTON%</button>
    <div id="customRulesMessage" class="message"></div>

    <details>
        <summary>Ajuda: Sintaxe das Regras Customizadas</summary>
        <div>
            <p><strong>Prefixos Principais:</strong></p>
            <ul>
                <li><code>DH HH:MM:SS</code>: <strong>Diário Alto (Ligar Saída)</strong> - Liga a saída todos os dias no horário especificado.</li>
                <li><code>DL HH:MM:SS</code>: <strong>Diário Baixo (Desligar Saída)</strong> - Desliga a saída todos os dias no horário especificado.</li>
                <li><code>WH d HH:MM:SS</code>: <strong>Semanal Alto (Ligar Saída)</strong> - Liga a saída no dia da semana <code>d</code> (1=Domingo, ..., 7=Sábado) no horário especificado.</li>
                <li><code>WL d HH:MM:SS</code>: <strong>Semanal Baixo (Desligar Saída)</strong> - Desliga a saída no dia da semana <code>d</code> no horário especificado.</li>
                <li><code>SH AAAA-MM-DD HH:MM</code>: <strong>Específico Alto (Ligar Saída)</strong> - Liga a saída na data e hora exatas. (Segundos não são usados aqui).</li>
                <li><code>SL AAAA-MM-DD HH:MM</code>: <strong>Específico Baixo (Desligar Saída)</strong> - Desliga a saída na data e hora exatas. (Segundos não são usados aqui).</li>
                <li><code>IH HH:MM:SS</code>: <strong>Intervalo Alto (Desligar após Ligado)</strong> - Se a saída estiver LIGADA, ela será DESLIGADA após o intervalo de tempo especificado.</li>
                <li><code>IL HH:MM:SS</code>: <strong>Intervalo Baixo (Ligar após Desligado)</strong> - Se a saída estiver DESLIGADA, ela será LIGADA após o intervalo de tempo especificado.</li>
            </ul>
            <p><small>Consulte a documentação completa para mais exemplos e detalhes.</small></p>
        </div>
    </details>

    <label for="eventConsole" style="margin-top:15px;">Console de Eventos:</label>
    <div id="eventConsole"></div>
  </section>

  <div id="nextTrigger">Carregando...</div>
  <div id="customRuleCountdown"></div>

  <script>
   function pad(n){ return n.toString().padStart(2,'0'); }
  function parseHHMMSS_to_secs(s){ const p=s.split(':').map(Number); return p[0]*3600+p[1]*60+p[2]; }
  function formatHHMMSS(secs){
    if (isNaN(secs) || secs < 0) return "00:00:00";
    return [Math.floor(secs/3600),Math.floor((secs%3600)/60),secs%60].map(pad).join(':');
  }

  function updateWifiQuality() {
    fetch('/rssi')
      .then(res => res.json())
      .then(j => {
        document.getElementById('wifiQuality').textContent = 'Wi-Fi: ' + j.pct + '%';
      })
      .catch(_=> {
        document.getElementById('wifiQuality').textContent = 'Wi-Fi: --%';
      });
  }
  setInterval(updateWifiQuality, 5000);
  updateWifiQuality();

  function updateCurrentTime() {
    fetch('/time')
        .then(res => res.text())
        .then(timeStr => {
            document.getElementById('currentTime').textContent = timeStr;
        })
        .catch(_ => {
            document.getElementById('currentTime').textContent = '--:--:--';
        });
  }
  setInterval(updateCurrentTime, 1000);
  updateCurrentTime();

  let schedules = [], manualIntervalSecs = parseHHMMSS_to_secs("%MANUAL%");
  const rawSch = "%SCHEDULES%";
  if(rawSch) schedules = rawSch.split(',').map(t=>{const a=t.split('|');return{time:a[0],interval:a[1]};});

  function renderSchedules(){
    const ul=document.getElementById('scheduleList'); ul.innerHTML='';
    schedules.forEach((o,i)=>{
      const li=document.createElement('li');
      li.innerHTML=`<span>${o.time} (Duração: ${o.interval})</span><button class="delete" data-index="${i}">×</button>`;
      li.querySelector('.delete').onclick=(e)=>{
          schedules.splice(parseInt(e.target.dataset.index),1);
          renderSchedules();
          updateNextTrigger();
      };
      ul.appendChild(li);
    });
  }
  renderSchedules();

  document.getElementById('addSchedule').onclick=()=>{
    const t=document.getElementById('newScheduleTime').value, i=document.getElementById('newScheduleInterval').value;
    const rx=/^([0-9]{2}):([0-9]{2}):([0-9]{2})$/;
    if(!t || !i) {showMessage('scheduleFormMessage','Preencha horário e duração','error');return;}
    if(!rx.test(t)||!rx.test(i)){showMessage('scheduleFormMessage','Use formato HH:MM:SS','error');return;}
    if (schedules.length >= 10) {
        showMessage('scheduleFormMessage', 'Máximo de 10 agendamentos atingido', 'error'); return;
    }
    schedules.push({time:t,interval:i});renderSchedules();
    showMessage('scheduleFormMessage','Agendamento adicionado localmente. Clique em "Salvar Agendamentos".','success');
  };

  document.getElementById('manualDurationForm').onsubmit=e=>{ // ID do formulário atualizado
    e.preventDefault();
    const d=document.getElementById('manualInterval').value, rx=/^([0-9]{2}):([0-9]{2}):([0-9]{2})$/;
    if(!rx.test(d)){showMessage('manualDurationMessage','Use formato HH:MM:SS','error');return;} // ID da mensagem atualizado
    fetch('/setManualDuration',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:`manualDuration=${d}`})
      .then(r=>{if(r.ok){showMessage('manualDurationMessage','Duração salva','success');manualIntervalSecs=parseHHMMSS_to_secs(d); } else {r.text().then(txt => showMessage('manualDurationMessage','Erro: ' + txt,'error'));}})
      .catch(_=>showMessage('manualDurationMessage','Erro ao salvar','error'));
  };

  document.getElementById('outputPinForm').onsubmit = e => { // ID do formulário atualizado
    e.preventDefault();
    const pin = document.getElementById('outputPinInput').value; // ID do input atualizado
    if (isNaN(parseInt(pin)) || parseInt(pin) < 0 ) {
      showMessage('outputPinMessage', 'Número do pino inválido', 'error'); // ID da mensagem atualizado
      return;
    }
    fetch('/setFeederPin', { // Endpoint mantido como /setFeederPin por simplicidade no backend
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: `feederPin=${pin}`
    })
    .then(r => {
      if (r.ok) {
        showMessage('outputPinMessage', 'Pino de saída salvo. Pode ser necessário reiniciar.', 'success');
      } else {
        r.text().then(txt => showMessage('outputPinMessage', 'Erro: ' + txt, 'error'));
      }
    })
    .catch(_ => showMessage('outputPinMessage', 'Erro ao salvar pino', 'error'));
  };

  document.getElementById('saveSchedules').onclick=()=>{
    const body='schedules='+encodeURIComponent(schedules.map(o=>`${o.time}|${o.interval}`).join(','));
    fetch('/setSchedules',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body})
      .then(r=>{if(r.ok){showMessage('scheduleFormMessage','Agendamentos salvos','success');updateNextTrigger();} else {r.text().then(txt => showMessage('scheduleFormMessage','Erro: ' + txt,'error'));}})
      .catch(_=>showMessage('scheduleFormMessage','Erro ao salvar','error'));
  };

  document.getElementById('saveRules').onclick=()=>{
    const rules=document.getElementById('customRulesInput').value;
    fetch('/setCustomRules',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'rules='+encodeURIComponent(rules)})
      .then(r=>{if(r.ok)showMessage('customRulesMessage','Regras salvas','success');else {r.text().then(txt => showMessage('customRulesMessage','Erro: ' + txt,'error'));}})
      .catch(_=>showMessage('customRulesMessage','Erro ao salvar','error'));
  };

  document.getElementById('toggleRules').onclick=()=>{
    fetch('/toggleCustomRules',{method:'POST'}).then(r=>{if(r.ok)location.reload();else {r.text().then(txt => showMessage('customRulesMessage','Erro: ' + txt,'error'));}}).catch(_=>showMessage('customRulesMessage','Erro ao alternar','error'));
  };

  const manualActivateButton = document.getElementById('manualActivateOutput'); // ID do botão atualizado
  const manualDeactivateButton = document.getElementById('manualDeactivateOutput'); // ID do botão atualizado

  manualActivateButton.onclick = () => {
    fetch('/feedNow', { method: 'POST' }) // Endpoint /feedNow mantido
        .then(r => {
            if (r.ok) {
                showMessage('manualOutputMessage', 'Saída ativada', 'success'); // ID da mensagem atualizado
            } else {
                return r.text().then(txt => { throw new Error(txt); });
            }
        })
        .catch(err => showMessage('manualOutputMessage', 'Erro: ' + err.message, 'error'))
        .finally(() => updateStatusAndRuleCountdown());
  };

  manualDeactivateButton.onclick = () => {
    fetch('/stopFeedNow', { method: 'POST' }) // Endpoint /stopFeedNow mantido
        .then(r => {
            if (r.ok) {
                showMessage('manualOutputMessage', 'Saída desativada.', 'success'); // ID da mensagem atualizado
            } else {
                 return r.text().then(txt => { throw new Error(txt); });
            }
        })
        .catch(err => showMessage('manualOutputMessage', 'Erro: ' + err.message, 'error'))
        .finally(() => updateStatusAndRuleCountdown());
  };

  let customRuleCountdownInterval = null;

  function updateStatusAndRuleCountdown() {
    fetch('/status')
        .then(res => res.json())
        .then(data => {
            if (data.is_feeding) { // is_feeding no backend representa isOutputActive
                manualDeactivateButton.style.display = 'block';
                manualActivateButton.disabled = true;
            } else {
                manualDeactivateButton.style.display = 'none';
                manualActivateButton.disabled = false;
            }

            if (customRuleCountdownInterval) clearInterval(customRuleCountdownInterval);
            const ruleCountdownElement = document.getElementById('customRuleCountdown');

            if (data.custom_rules_enabled && data.active_custom_rule !== "none" && data.custom_rule_time_remaining >= 0) {
                ruleCountdownElement.style.display = 'block';
                let totalSeconds = data.custom_rule_time_remaining;
                let ruleType = data.active_custom_rule;
                let ruleState = ruleType === "IH" ? "LIGADO" : "DESLIGADO";

                ruleCountdownElement.textContent = `Regra Ativa (${ruleType}): Tempo ${ruleState} restante: ${formatHHMMSS(totalSeconds)}`;

                customRuleCountdownInterval = setInterval(() => {
                    if (totalSeconds <= 0) {
                        clearInterval(customRuleCountdownInterval);
                        ruleCountdownElement.textContent = "Regra Ativa: Verificando...";
                        setTimeout(updateStatusAndRuleCountdown, 1500);
                        return;
                    }
                    totalSeconds--;
                    ruleCountdownElement.textContent = `Regra Ativa (${ruleType}): Tempo ${ruleState} restante: ${formatHHMMSS(totalSeconds)}`;
                }, 1000);
            } else {
                ruleCountdownElement.style.display = 'none';
            }
        })
        .catch(_ => {
            manualDeactivateButton.style.display = 'none';
            manualActivateButton.disabled = false;
            if (customRuleCountdownInterval) clearInterval(customRuleCountdownInterval);
            document.getElementById('customRuleCountdown').style.display = 'none';
        });
  }
  setInterval(updateStatusAndRuleCountdown, 2500);
  updateStatusAndRuleCountdown();

  let countdownInterval = null;
  function updateNextTrigger(){
    fetch('/nextTriggerTime')
        .then(res => res.text())
        .then(text => {
            if (countdownInterval) clearInterval(countdownInterval);
            const triggerElement = document.getElementById('nextTrigger');

            const match = text.match(/Próxima em: (\d{2}):(\d{2}):(\d{2})/);
            const durationMatch = text.match(/\(duração (\d{2}):(\d{2}):(\d{2})\)/);
            let originalSuffix = "";
            if (durationMatch) {
                originalSuffix = ` (duração ${durationMatch[1]}:${durationMatch[2]}:${durationMatch[3]})`;
            } else if (match) {
                const afterTime = text.substring(text.indexOf(match[0]) + match[0].length);
                if (afterTime.trim().length > 0) originalSuffix = afterTime;
            }

            if (match) {
                let hours = parseInt(match[1]);
                let minutes = parseInt(match[2]);
                let seconds = parseInt(match[3]);
                let totalSeconds = hours * 3600 + minutes * 60 + seconds;

                if (totalSeconds >= 0) {
                    triggerElement.textContent = `Próxima em: ${pad(hours)}:${pad(minutes)}:${pad(seconds)}${originalSuffix}`;
                    countdownInterval = setInterval(() => {
                        if (totalSeconds <= 0) {
                            clearInterval(countdownInterval);
                            triggerElement.textContent = "Verificando próximo agendamento...";
                            setTimeout(updateNextTrigger, 1500);
                            updateStatusAndRuleCountdown();
                            return;
                        }
                        totalSeconds--;
                        const h = Math.floor(totalSeconds / 3600);
                        const m = Math.floor((totalSeconds % 3600) / 60);
                        const s = totalSeconds % 60;
                        triggerElement.textContent = `Próxima em: ${pad(h)}:${pad(m)}:${pad(s)}${originalSuffix}`;
                    }, 1000);
                } else {
                     triggerElement.textContent = text;
                }
            } else {
                 triggerElement.textContent = text;
            }
        })
        .catch(_ => {
            if (countdownInterval) clearInterval(countdownInterval);
            document.getElementById('nextTrigger').textContent = 'Erro ao buscar próximo acionamento.';
        });
  }
  setInterval(updateNextTrigger, 30000);
  updateNextTrigger();

  function showMessage(id,msg,type){
    const e=document.getElementById(id);e.textContent=msg;e.className='message '+type;
    setTimeout(()=>{e.textContent='';e.className='message';},5000);
  }

  setInterval(() => {
    fetch('/events')
      .then(res => res.text())
      .then(txt => {
        if (txt && txt.trim().length > 0) {
          const con = document.getElementById('eventConsole');
          const needsScroll = con.scrollHeight - con.scrollTop === con.clientHeight;
          con.innerText += (con.innerText ? '\n' : '') + txt.trim();
          if(needsScroll) con.scrollTop = con.scrollHeight;
        }
      });
  }, 2000);
    
      function start(e) {
        drawing = true;
        const p = getPos(e);
        lastX = p.x; lastY = p.y;
        e.preventDefault();
      }

      function draw(e) {
        if (!drawing) return;
        const p = getPos(e);
        ctx.beginPath();
        ctx.moveTo(lastX, lastY);
        ctx.lineTo(p.x, p.y);
        ctx.stroke();
        lastX = p.x; lastY = p.y;
        e.preventDefault();
      }

      function stop(e) {
        drawing = false;
        e.preventDefault();
      }

      canvas.addEventListener('mousedown', start);
      canvas.addEventListener('touchstart', start);
      canvas.addEventListener('mousemove', draw);
      canvas.addEventListener('touchmove', draw);
      canvas.addEventListener('mouseup', stop);
      canvas.addEventListener('mouseout', stop);
      canvas.addEventListener('touchend', stop);

      window.clearBoard = function() {
        ctx.clearRect(0, 0, canvas.width, canvas.height);
      };
    })();
  </script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
