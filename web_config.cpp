#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "bms_ble.h"
#include "wifi_switch.h"
#include "config.h"

extern JKBMS* jkBMS;
extern NimBLEScan* pScan;

WebServer server(WEB_SERVER_PORT);
Preferences prefs;

WiFiClient eventClient;
bool clientConnected = false;

void initWebServer(){
  prefs.begin("bms_config", false);

  initWiFiAP();

  server.on("/", [](){

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>JK-BMS Setup</title>

<style>
:root {
  --bg: #0f172a;
  --card: #1e293b;
  --accent: #38bdf8;
  --text: #e2e8f0;
  --muted: #94a3b8;
}

body {
  margin:0;
  font-family: system-ui;
  background: var(--bg);
  color: var(--text);
}

.container {
  max-width: 500px;
  margin: auto;
  padding: 16px;
}

.card {
  background: var(--card);
  border-radius: 16px;
  padding: 16px;
  margin-bottom: 16px;
}

button {
  width: 100%;
  padding: 12px;
  margin-top: 8px;
  border-radius: 10px;
  border: none;
  background: var(--accent);
  color: black;
  font-weight: bold;
  font-size: 16px;
}

button.secondary {
  background: #334155;
  color: white;
}

input.mac {
  width: 100%;
  padding: 10px;
  text-align: center;
  border-radius: 8px;
  border: none;
  font-size: 16px;
}

.hidden {
  display: none;
}

.small {
  font-size: 12px;
  color: var(--muted);
}
</style>
</head>

<body>

<div class="container">

<div class="card">
  <h2>🔋 JK-BMS Setup</h2>
  <div class="small">Saved MAC:</div>
  <div id="savedMac">-</div>
</div>

<div class="card">
  <button onclick="startScanRealtime()">▶️ Start Scan</button>
  <button class="secondary" onclick="stopScanRealtime()">⏹ Stop Scan</button>
  <button class="secondary" onclick="toggleManual()">✍️ Enter Manually</button>
</div>

<div class="card hidden" id="manualForm">
  <h3>Enter MAC Address</h3>
  <input class="mac" id="macInput" maxlength="17" placeholder="000000000000 or 00:00:00:00:00:00">
  <button onclick="submitManual()">Save MAC</button>
</div>

<div class="card">
  <h3>Devices</h3>
  <div id="deviceList"></div>
</div>

<!-- ================= WIFI CONTROL (ADDED ONLY) ================= -->
<div class="card">
  <h3>⚙️ WiFi Control</h3>

  <label style="display:flex;align-items:center;gap:10px;">
    <input type="checkbox" id="keepWifi">
    <span style="font-size:14px;">
      Tetap hidupkan WiFi meskipun BMS terhubung
    </span>
  </label>

  <button onclick="saveWifiMode()">Save Setting</button>
</div>

</div>

<script>
let devices = [];
let eventSource = null;

fetch('/api/status')
  .then(res => res.text())
  .then(mac => document.getElementById('savedMac').innerText = mac);

function toggleManual() {
  document.getElementById("manualForm").classList.toggle("hidden");
}

// ================= MAC INPUT =================
function submitManual() {
  let raw = document.getElementById("macInput").value;
  let clean = raw.replace(/[^a-fA-F0-9]/g, "").toLowerCase();

  if (clean.length !== 12) {
    alert("MAC harus 12 karakter hex");
    return;
  }

  let mac = clean.match(/.{2}/g).join(":");

  fetch('/api/select', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'mac=' + mac
  });
}

// ================= WIFI CONTROL (ADDED ONLY) =================
function saveWifiMode() {
  let v = document.getElementById("keepWifi").checked ? "1" : "0";

  fetch('/api/wifi_mode', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'keep=' + v
  });
}

fetch('/api/wifi_mode')
  .then(r => r.text())
  .then(v => {
    document.getElementById("keepWifi").checked = (v === "1");
  });

// ================= SCAN (UNCHANGED) =================
function startScanRealtime() {
  document.getElementById('deviceList').innerHTML = "";
  devices = [];

  fetch('/api/scan/start');

  eventSource = new EventSource('/api/scan/stream');

  eventSource.onmessage = function(event) {
    let device = JSON.parse(event.data);

    if (!devices.find(d => d.address === device.address)) {
      devices.push(device);
      addDevice(device);
    }
  };
}

function stopScanRealtime() {
  fetch('/api/scan/stop');
  if (eventSource) eventSource.close();
}

function addDevice(device) {
  let div = document.createElement("div");

  div.innerHTML = `
    <b>${device.name}</b><br>
    <span class="small">${device.address}</span><br>
    <span class="small">RSSI: ${device.rssi}</span><br>
    <button onclick="selectDevice('${device.address}')">Select</button>
    <hr>
  `;

  document.getElementById("deviceList").appendChild(div);
}

function selectDevice(mac) {
  if (!confirm("Gunakan device ini?\n" + mac)) return;

  fetch('/api/select', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'mac=' + mac
  });
}
</script>

</body>
</html>
)rawliteral";

    server.send(200,"text/html",html);
  });

  // ================= SSE =================
  server.on("/api/scan/stream", HTTP_GET, []() {
    WiFiClient client = server.client();

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/event-stream");
    client.println("Cache-Control: no-cache");
    client.println("Connection: keep-alive");
    client.println();

    eventClient = client;
    clientConnected = true;
  });

  // ================= SCAN =================
  server.on("/api/scan/start", [](){
    clearScanResults();
    startBLEScan(0);
    server.send(200, "text/plain", "started");
  });

  server.on("/api/scan/stop", [](){
    if (pScan->isScanning()) pScan->stop();
    server.send(200, "text/plain", "stopped");
  });

  // ================= SAVE MAC =================
  server.on("/api/select", HTTP_POST, [](){
    if (server.hasArg("mac")) {
      String mac = server.arg("mac");
      mac.toLowerCase();

      prefs.putString("bms_mac", mac);

      server.send(200, "text/plain", "OK");
      delay(500);
      ESP.restart();
    }
  });

  // ================= STATUS =================
  server.on("/api/status", [](){
    server.send(200, "text/plain", prefs.getString("bms_mac", "not set"));
  });

  // ================= WIFI CONTROL API =================
  server.on("/api/wifi_mode", HTTP_GET, [](){
    bool v = prefs.getBool("keep_wifi", false);
    server.send(200, "text/plain", v ? "1" : "0");
  });

  server.on("/api/wifi_mode", HTTP_POST, [](){
    bool v = server.arg("keep") == "1";
    prefs.putBool("keep_wifi", v);
    keepWiFiOn = v;
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

void handleWebServer(){
  server.handleClient();
}
