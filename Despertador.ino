#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Arduino.h>

// --- CONFIGURACIÓN DE PINES ---
#define R_PIN   17
#define G_PIN   16
#define B_PIN   5
#define WW_PIN  19
#define CW_PIN  18

// --- CREDENCIALES WIFI ---
const char* ssid     = "";
const char* password = "";

// --- CONFIGURACIÓN IP ESTÁTICA ---
IPAddress local_IP(192, 168, 1, 38);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); 

// --- SERVIDOR WEB ---
WebServer server(80);

// --- NTP (HORA) ---
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;      // GMT+1 (Cambiar a 0 o 7200 según zona)
const int daylightOffset_sec = 3600;  // Horario verano

// --- VARIABLES DE ESTADO ---
bool amanecerEnProgreso = false;
bool activarAutomaticamente = true;
unsigned long inicioAmanecerMillis = 0;
const int DURACION_AMANECER_MIN = 45;

int horaAlarma = 7;
int minutoAlarma = 30;

// Variables para modo manual
int r_man = 0, g_man = 0, b_man = 0, ww_man = 0, cw_man = 0;
bool modoManual = false;

// --- FUNCIONES AUXILIARES ---

int interpolar(int t, int t1, int t2, int v1, int v2) {
  if (t < t1) return v1;
  if (t > t2) return v2;
  return map(t, t1, t2, v1, v2);
}

void apagarTodo() {
  analogWrite(R_PIN, 0); analogWrite(G_PIN, 0); analogWrite(B_PIN, 0);
  analogWrite(WW_PIN, 0); analogWrite(CW_PIN, 0);
  r_man = g_man = b_man = ww_man = cw_man = 0;
}

void detenerProcesos() {
  amanecerEnProgreso = false;
  modoManual = false;
  apagarTodo();
}

void iniciarAmanecer() {
  if (amanecerEnProgreso) return;
  amanecerEnProgreso = true;
  modoManual = false;
  inicioAmanecerMillis = millis();
}

void ejecutarSecuenciaAmanecer() {
  unsigned long elapsed = millis() - inicioAmanecerMillis;
  int minutos = elapsed / 60000;

  if (minutos >= DURACION_AMANECER_MIN + 1) { detenerProcesos(); return; }
  if (minutos > DURACION_AMANECER_MIN) { detenerProcesos(); return; }

  int r = (minutos <= 10) ? interpolar(minutos, 0, 10, 20, 60) :
          (minutos <= 20) ? interpolar(minutos, 10, 20, 60, 90) :
          (minutos <= 35) ? interpolar(minutos, 20, 35, 90, 80) : interpolar(minutos, 35, 45, 80, 40);

  int g = (minutos <= 10) ? 0 :
          (minutos <= 20) ? interpolar(minutos, 10, 20, 10, 30) :
          (minutos <= 35) ? interpolar(minutos, 20, 35, 30, 40) : interpolar(minutos, 35, 45, 40, 50);

  int b = (minutos <= 20) ? interpolar(minutos, 10, 20, 0, 10) :
          (minutos <= 35) ? interpolar(minutos, 20, 35, 10, 20) : interpolar(minutos, 35, 45, 20, 30);

  int ww = (minutos <= 20) ? interpolar(minutos, 10, 20, 20, 80) :
           (minutos <= 35) ? interpolar(minutos, 20, 35, 80, 150) : interpolar(minutos, 35, 45, 150, 180);

  int cw = (minutos <= 35) ? interpolar(minutos, 20, 35, 0, 30) : interpolar(minutos, 35, 45, 30, 100);

  analogWrite(R_PIN, constrain(r, 0, 255));
  analogWrite(G_PIN, constrain(g, 0, 255));
  analogWrite(B_PIN, constrain(b, 0, 255));
  analogWrite(WW_PIN, constrain(ww, 0, 255));
  analogWrite(CW_PIN, constrain(cw, 0, 255));
}

// --- HTML PAGINA WEB ---
String getHTML() {
  String check = activarAutomaticamente ? "checked" : "";
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", horaAlarma, minutoAlarma);

  String html = R"rawliteral(
<!DOCTYPE html><html lang="es"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Control Amanecer</title>
<style>
  body{font-family:sans-serif;margin:0;padding:20px;background:#232931;color:#EEE;text-align:center;}
  .container{max-width:400px;margin:auto;padding:20px;background:#393E46;border-radius:15px;box-shadow:0 4px 12px rgba(0,0,0,0.2);}
  input[type="time"]{font-size:24px;padding:10px;border-radius:8px;border:none;margin:10px 0;width:80%;text-align:center;}
  .btn{padding:15px;width:100%;font-size:18px;border:none;border-radius:8px;margin-top:10px;cursor:pointer;color:white;}
  .stop{background:#e74c3c;}.stop:hover{background:#c0392b;}
  .read{background:#3498db;}.read:hover{background:#2980b9;}
  .slider{width:100%;margin:15px 0;}
  label{display:block;margin-top:15px;}
  .status-text { font-size: 14px; color: #4cd137; margin-top: 5px; min-height: 20px;}
  input[type=range]{-webkit-appearance:none;width:100%;background:transparent;}
  input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;height:20px;width:20px;border-radius:50%;background:#fff;cursor:pointer;margin-top:-8px;}
  input[type=range]::-webkit-slider-runnable-track{width:100%;height:4px;cursor:pointer;border-radius:2px;}
  #r::-webkit-slider-runnable-track{background:red;}
  #g::-webkit-slider-runnable-track{background:green;}
  #b::-webkit-slider-runnable-track{background:blue;}
  #ww::-webkit-slider-runnable-track{background:#ffcc80;}
  #cw::-webkit-slider-runnable-track{background:#b3e5fc;}
</style>
<script>
function send(url) { fetch(url, {method:'POST'}); }

// Función mejorada para actualizar hora con feedback
function setTime(val) { 
  document.getElementById('conf').innerText = "Guardando...";
  document.getElementById('conf').style.color = "#f1c40f"; // Amarillo mientras guarda
  
  fetch('/sethora?t='+val, {method:'POST'})
    .then(response => response.text())
    .then(data => {
      // Data contiene la hora que devuelve el ESP32
      document.getElementById('conf').innerText = "✅ Confirmado: " + data;
      document.getElementById('conf').style.color = "#4cd137"; // Verde confirmado
    })
    .catch(error => {
      document.getElementById('conf').innerText = "❌ Error de conexión";
      document.getElementById('conf').style.color = "#e74c3c";
    });
}

function setAuto(cb) { fetch('/setauto?v='+(cb.checked?'1':'0'), {method:'POST'}); }
function setColor(ch, val) { fetch('/setcolor?c='+ch+'&v='+val, {method:'POST'}); }
</script>
</head><body>
<div class="container">
  <h2>⏰ Despertador</h2>
  <input type="time" value=")rawliteral" + String(timeStr) + R"rawliteral(" onchange="setTime(this.value)">
  <div id="conf" class="status-text">Hora actual guardada: )rawliteral" + String(timeStr) + R"rawliteral(</div>
  <br>
  <label><input type="checkbox" onchange="setAuto(this)" )rawliteral" + check + R"rawliteral(> Activación Automática</label>
  <button class="btn stop" onclick="send('/stop')">APAGAR TODO (STOP)</button>
</div>
<div class="container" style="margin-top:20px;">
  <h3>💡 Lámpara Manual</h3>
  <input id="r" class="slider" type="range" min="0" max="255" value="0" oninput="setColor('r',this.value)">
  <input id="g" class="slider" type="range" min="0" max="255" value="0" oninput="setColor('g',this.value)">
  <input id="b" class="slider" type="range" min="0" max="255" value="0" oninput="setColor('b',this.value)">
  <input id="ww" class="slider" type="range" min="0" max="255" value="0" oninput="setColor('ww',this.value)">
  <input id="cw" class="slider" type="range" min="0" max="255" value="0" oninput="setColor('cw',this.value)">
  <button class="btn read" onclick="send('/readmode')">Modo Lectura</button>
</div>
</body></html>
)rawliteral";
  return html;
}

// --- MANEJADORES DE RUTAS ---
void handleRoot() { server.send(200, "text/html", getHTML()); }
void handleStop() { detenerProcesos(); server.send(200, "text/plain", "STOP OK"); }

void handleSetHora() {
  if (server.hasArg("t")) {
    String t = server.arg("t");
    horaAlarma = t.substring(0, 2).toInt();
    minutoAlarma = t.substring(3, 5).toInt();
    Serial.printf("Nueva alarma: %02d:%02d\n", horaAlarma, minutoAlarma);
  }
  // DEVOLVEMOS LA HORA REALMENTE GUARDADA
  char confirm[10];
  snprintf(confirm, sizeof(confirm), "%02d:%02d", horaAlarma, minutoAlarma);
  server.send(200, "text/plain", confirm);
}

void handleSetAuto() {
  if (server.hasArg("v")) activarAutomaticamente = (server.arg("v") == "1");
  server.send(200, "text/plain", "OK");
}

void handleSetColor() {
  if (server.hasArg("c") && server.hasArg("v")) {
    String c = server.arg("c");
    int v = server.arg("v").toInt();
    if (c == "r") r_man = v; else if (c == "g") g_man = v; else if (c == "b") b_man = v;
    else if (c == "ww") ww_man = v; else if (c == "cw") cw_man = v;
    modoManual = true; amanecerEnProgreso = false;
    analogWrite(R_PIN, r_man); analogWrite(G_PIN, g_man); analogWrite(B_PIN, b_man);
    analogWrite(WW_PIN, ww_man); analogWrite(CW_PIN, cw_man);
  }
  server.send(200, "text/plain", "OK");
}

void handleReadMode() {
  r_man = 0; g_man = 0; b_man = 0; ww_man = 150; cw_man = 80;
  modoManual = true; amanecerEnProgreso = false;
  analogWrite(R_PIN, r_man); analogWrite(G_PIN, g_man); analogWrite(B_PIN, b_man);
  analogWrite(WW_PIN, ww_man); analogWrite(CW_PIN, cw_man);
  server.send(200, "text/plain", "OK");
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) Serial.println("Fallo IP fija");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConectado: " + WiFi.localIP().toString());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/sethora", HTTP_POST, handleSetHora);
  server.on("/setauto", HTTP_POST, handleSetAuto);
  server.on("/setcolor", HTTP_POST, handleSetColor);
  server.on("/readmode", HTTP_POST, handleReadMode);
  server.begin();
  
  apagarTodo();
}

void loop() {
  server.handleClient();

  if (amanecerEnProgreso) ejecutarSecuenciaAmanecer();

  if (activarAutomaticamente && !amanecerEnProgreso) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      if (timeinfo.tm_hour == horaAlarma && timeinfo.tm_min == minutoAlarma && timeinfo.tm_sec == 0) {
        iniciarAmanecer();
      }
    }
  }
}
