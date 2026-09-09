//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_SERVICES_WEBUI_H
#define FRIENDLYBOT_SERVICES_WEBUI_H

#ifndef PROGMEM
#define PROGMEM
#endif

namespace WebUi
{
static constexpr const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>FriendlyBot | Console50</title>
<style>
:root {
  --bg: #121214;
  --card-bg: #1c1c20;
  --yellow: #ffcc00;
  --blue: #00d2ff;
  --text: #e0e0e0;
  --text-muted: #888;
  --border: #333;
}
* { box-sizing: border-box; }
body {
  margin: 0; padding: 20px;
  background: var(--bg); color: var(--text);
  font-family: system-ui, -apple-system, sans-serif;
}
.container {
  max-width: 800px; margin: 0 auto;
}
.header {
  display: flex; justify-content: space-between; align-items: center;
  flex-wrap: wrap; margin-bottom: 20px;
}
.header h1 {
  color: var(--yellow); margin: 0;
  font-size: 24px;
}
.status-pills {
  display: flex; gap: 10px; flex-wrap: wrap;
}
.pill {
  background: var(--card-bg); border: 1px solid var(--border);
  padding: 4px 10px; border-radius: 12px; font-size: 14px;
}
.pill span { color: var(--blue); }
.card {
  background: var(--card-bg); border: 1px solid var(--border);
  border-radius: 8px; padding: 20px; margin-bottom: 20px;
}
.card h2 {
  margin-top: 0; color: var(--blue); font-size: 18px; border-bottom: 1px solid var(--border); padding-bottom: 10px;
}
.card h3 {
  margin: 20px 0 10px 0; color: var(--yellow); font-size: 15px; border-bottom: 1px dashed var(--border); padding-bottom: 5px;
}
.checkbox-label {
  display: flex; align-items: center; gap: 8px; cursor: pointer; font-weight: normal; margin-bottom: 0;
}
.checkbox-label input[type="checkbox"] {
  cursor: pointer;
}
.form-group {
  margin-bottom: 15px;
}
label {
  display: block; margin-bottom: 5px; font-weight: 500; font-size: 14px;
}
input[type="text"], input[type="password"], input[type="number"], select, textarea {
  width: 100%; padding: 10px;
  background: var(--bg); border: 1px solid var(--border);
  color: var(--text); border-radius: 4px;
  font-family: inherit; font-size: 14px;
}
.input-group {
  display: flex; gap: 10px;
}
.input-group input, .input-group select {
  flex: 1;
}
button {
  background: var(--card-bg); border: 1px solid var(--border);
  color: var(--text); padding: 10px 15px; cursor: pointer;
  border-radius: 4px; font-weight: 600; font-size: 14px;
  transition: all 0.2s;
}
button:hover { background: var(--bg); border-color: var(--blue); }
button.primary { background: var(--blue); color: var(--bg); border: none; }
button.primary:hover { background: #00b0d8; }
button.warning { background: transparent; border-color: #ff4444; color: #ff4444; }
button.warning:hover { background: #ff4444; color: white; }
.btn-group { display: flex; gap: 10px; flex-wrap: wrap; }
.radio-group {
  display: flex; gap: 10px;
}
.radio-group label {
  display: flex; align-items: center; gap: 5px; cursor: pointer;
}
.diag-table {
  width: 100%; border-collapse: collapse;
}
.diag-table th, .diag-table td {
  padding: 8px; text-align: left; border-bottom: 1px solid var(--border);
}
.diag-table th { color: var(--text-muted); }
.progress {
  width: 100%; background: var(--bg); border-radius: 4px; height: 20px; overflow: hidden; margin-top: 10px; display: none;
}
.progress-bar {
  width: 0%; height: 100%; background: var(--blue);
}
#toast {
  position: fixed; bottom: 20px; right: 20px;
  background: var(--card-bg); border: 1px solid var(--yellow);
  padding: 15px 20px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.3);
  display: none; z-index: 1000;
}
@media (max-width: 600px) {
  .header { flex-direction: column; align-items: flex-start; gap: 10px; }
  .input-group { flex-direction: column; }
}
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <h1>FriendlyBot <span style="color: var(--blue);">| Console50</span></h1>
    <div class="status-pills">
      <div class="pill">Status: <span id="status-mode">Loading...</span></div>
      <div class="pill">IP: <span id="status-ip">-</span></div>
      <div class="pill">RSSI: <span id="status-rssi">-</span></div>
      <div class="pill">Heap: <span id="status-heap">-</span></div>
      <div class="pill">Uptime: <span id="status-uptime">-</span></div>
    </div>
  </div>

  <div class="card">
    <h2>1. WiFi Setup</h2>
    <div class="form-group">
      <div class="btn-group" style="margin-bottom: 10px;">
        <button type="button" onclick="scanWifi()">Scan Networks</button>
      </div>
      <select id="wifi-select" onchange="document.getElementById('wifi-ssid').value = this.value;"></select>
    </div>
    <div class="form-group input-group">
      <input type="text" id="wifi-ssid" placeholder="SSID">
      <input type="password" id="wifi-pass" placeholder="Password">
      <button type="button" onclick="togglePass('wifi-pass')">Show</button>
    </div>
    <div class="btn-group">
      <button class="primary" onclick="saveWifi()">Connect / Save WiFi</button>
      <button class="warning" onclick="clearWifi()">Clear Credentials</button>
    </div>
  </div>

  <div class="card">
    <h2>2. Device & Location</h2>
    <div class="form-group">
      <label>Hostname</label>
      <input type="text" id="dev-hostname" value="bot50">
    </div>
    <div class="form-group" style="display:flex; gap:10px;">
      <div style="flex:1">
        <label>Latitude</label>
        <input type="number" step="any" id="dev-lat" placeholder="e.g. 45.4642">
      </div>
      <div style="flex:1">
        <label>Longitude</label>
        <input type="number" step="any" id="dev-lon" placeholder="e.g. 9.1900">
      </div>
    </div>
    <div class="form-group input-group">
      <div style="flex:1">
        <label>OpenWeather API Key</label>
        <div style="display:flex; gap:10px;">
            <input type="password" id="dev-api_key" placeholder="API Key">
            <button type="button" onclick="togglePass('dev-api_key')">Show</button>
        </div>
      </div>
    </div>
    <div class="form-group">
      <label>Timezone / POSIX Override</label>
      <div class="input-group">
        <select id="tz-preset" onchange="if(this.value) document.getElementById('dev-tz').value = this.value;">
          <option value="">-- Select Preset --</option>
          <option value="CET-1CEST,M3.5.0,M10.5.0/3">Rome (CET/CEST)</option>
          <option value="GMT0BST,M3.5.0/1,M10.5.0">London (GMT/BST)</option>
          <option value="EST5EDT,M3.2.0,M11.1.0">New York (EST/EDT)</option>
          <option value="JST-9">Tokyo (JST)</option>
          <option value="">Custom</option>
        </select>
        <input type="text" id="dev-tz" placeholder="CET-1CEST,M3.5.0,M10.5.0/3">
      </div>
    </div>
    <button class="primary" onclick="saveConfig(['dev-hostname', 'dev-lat', 'dev-lon', 'dev-api_key', 'dev-tz'])">Save Location & Device</button>
  </div>

  <div class="card">
    <h2>3. Preferences</h2>
    <div class="form-group">
      <label>System Language</label>
      <div class="radio-group">
        <label><input type="radio" name="pref-lang" value="en" checked> English 🇬🇧</label>
        <label><input type="radio" name="pref-lang" value="it"> Italiano 🇮🇹</label>
      </div>
    </div>
    <!--div class="form-group">
      <label>Timer Visual Mode</label>
      <div class="radio-group">
        <label><input type="radio" name="pref-timer" value="0" checked> Digits with Bar</label>
        <label><input type="radio" name="pref-timer" value="1"> Digital Sand (Gravity)</label>
      </div>
    </div-->
    <h3>Hardware Calibration & Sensitivity</h3>
    <!--div class="form-group">
      <label>Temperature Offset (°C)</label>
      <input type="number" id="pref-temp-offset" step="0.1" value="0.0">
    </div-->
    <div class="form-group">
      <label>Night Mode Light Threshold (0–4095)</label>
      <input type="number" id="pref-night-thresh" min="0" max="4095" step="10" value="300">
    </div>
    <div class="form-group">
      <label>Slap Detection Threshold (G)</label>
      <input type="number" id="pref-slap-thresh" min="0.5" max="5.0" step="0.1" value="2.0">
    </div>
    <h3>Hardware Orientation & Polarity</h3>
    <div class="form-group">
      <label class="checkbox-label"><input type="checkbox" id="pref-inv-pot-l"> Invert Left Potentiometer (Kitchen Timer)</label>
    </div>
    <div class="form-group">
      <label class="checkbox-label"><input type="checkbox" id="pref-inv-pot-r"> Invert Right Potentiometer (Arbitrator)</label>
    </div>
    <div class="form-group">
      <label class="checkbox-label"><input type="checkbox" id="pref-inv-pwr"> Invert Power Sense Polarity (GPIO 4: USB vs Battery)</label>
    </div>
    <div class="form-group" style="margin-top: 15px;">
      <label>Default Orientation Calibration</label>
      <div class="btn-group" style="align-items: center; gap: 12px;">
        <button type="button" onclick="saveOrientation()">Save Current Orientation as Down</button>
        <span id="pref-orient-status" class="pill">Status: <span id="pref-orient-text">Not Calibrated</span></span>
      </div>
    </div>
    <button class="primary" onclick="savePreferences()">Save Preferences</button>
  </div>

  <div class="card">
    <h2>4. Custom Aphorisms & Sentences (LittleFS)</h2>
    <div class="status-pills" style="margin-bottom: 10px;">
      <div class="pill">Status: <span id="sent-status">Using Factory Built-in</span></div>
      <div class="pill">Storage: <span id="sent-quota">0 / 110 KB (0%)</span></div>
    </div>
    <div class="form-group">
      <textarea id="sent-text" rows="6" placeholder="One sentence per line..."></textarea>
    </div>
    <div class="btn-group">
      <button class="primary" onclick="saveSentences()">Save Custom Sentences</button>
      <button onclick="loadTemplate()">Load Built-in Template</button>
      <button class="warning" onclick="deleteSentences()">Delete Custom & Use Built-in</button>
    </div>
  </div>

  <div class="card">
    <h2>5. System & Native OTA Update</h2>
    <table class="diag-table" style="margin-bottom: 15px;">
      <tr><th>Chip</th><td id="sys-chip">-</td></tr>
      <tr><th>Flash</th><td id="sys-flash">-</td></tr>
      <tr><th>Free Heap</th><td id="sys-freeheap">-</td></tr>
    </table>
    <div class="form-group">
      <input type="file" id="ota-file" accept=".bin">
    </div>
    <div class="progress" id="ota-progress-container"><div class="progress-bar" id="ota-progress"></div></div>
    <div class="btn-group" style="margin-top: 15px;">
      <button class="primary" onclick="doOTA()">Upload & Update</button>
      <button class="warning" onclick="restartBot()">Restart Bot</button>
    </div>
  </div>
</div>
<div id="toast"></div>

<script>
function showToast(msg) {
  const t = document.getElementById('toast');
  t.innerText = msg; t.style.display = 'block';
  setTimeout(() => t.style.display = 'none', 3000);
}
function togglePass(id) {
  const i = document.getElementById(id);
  i.type = i.type === 'password' ? 'text' : 'password';
}

function updateStatus() {
  fetch('/api/status').then(r => r.json()).then(d => {
    document.getElementById('status-mode').innerText = d.mode || '-';
    document.getElementById('status-ip').innerText = d.ip || '-';
    document.getElementById('status-rssi').innerText = d.rssi ? (d.rssi + ' ▂▄▆█') : '-';
    document.getElementById('status-heap').innerText = d.heap || '-';
    document.getElementById('status-uptime').innerText = d.uptime || '-';
    document.getElementById('sys-chip').innerText = d.chip || '-';
    document.getElementById('sys-flash').innerText = d.flash || '-';
    document.getElementById('sys-freeheap').innerText = d.heap || '-';
    
    if(d.hostname !== undefined) document.getElementById('dev-hostname').value = d.hostname;
    else if(d.host !== undefined) document.getElementById('dev-hostname').value = d.host;

    if(d.lat !== undefined) document.getElementById('dev-lat').value = d.lat;
    if(d.lon !== undefined) document.getElementById('dev-lon').value = d.lon;

    if(d.api_key !== undefined) document.getElementById('dev-api_key').value = d.api_key;
    else if(d.apiKey !== undefined) document.getElementById('dev-api_key').value = d.apiKey;
    else if(d.apikey !== undefined) document.getElementById('dev-api_key').value = d.apikey;

    const tzVal = (d.tz !== undefined) ? d.tz : d.timezone;
    if(tzVal !== undefined) {
      document.getElementById('dev-tz').value = tzVal;
      const tzSelect = document.getElementById('tz-preset');
      if(tzSelect) {
        let matched = false;
        for(let i = 0; i < tzSelect.options.length; ++i) {
          if(tzSelect.options[i].value && tzSelect.options[i].value === tzVal) {
            tzSelect.selectedIndex = i;
            matched = true;
            break;
          }
        }
        if(!matched) tzSelect.value = '';
      }
    }

    if(d.lang) {
      const langRadio = document.querySelector(`input[name="pref-lang"][value="${d.lang}"]`);
      if(langRadio) langRadio.checked = true;
    }

    /*const tm = (d.timer_mode !== undefined) ? d.timer_mode : d.timerMode;
    if(tm !== undefined && tm !== null) {
      const timerRadio = document.querySelector(`input[name="pref-timer"][value="${tm}"]`);
      if(timerRadio) timerRadio.checked = true;
    }*/

    //if(d.temp_offset !== undefined && d.temp_offset !== null) document.getElementById('pref-temp-offset').value = Number(d.temp_offset).toFixed(1);
    if(d.night_threshold !== undefined && d.night_threshold !== null) document.getElementById('pref-night-thresh').value = d.night_threshold;
    if(d.slap_threshold !== undefined && d.slap_threshold !== null) document.getElementById('pref-slap-thresh').value = Number(d.slap_threshold).toFixed(1);
    if(d.invert_pot_l !== undefined) document.getElementById('pref-inv-pot-l').checked = d.invert_pot_l;
    if(d.invert_pot_r !== undefined) document.getElementById('pref-inv-pot-r').checked = d.invert_pot_r;
    if(d.invert_pwr !== undefined) document.getElementById('pref-inv-pwr').checked = d.invert_pwr;
    if(d.orientation_calibrated !== undefined) {
      const txt = document.getElementById('pref-orient-text');
      if(txt) {
        txt.innerText = d.orientation_calibrated ? 'Calibrated' : 'Not Calibrated';
        txt.style.color = d.orientation_calibrated ? '#44ff44' : 'var(--blue)';
      }
    }
  }).catch(e => console.error(e));
}

function loadSentencesStatus() {
  fetch('/api/sentences').then(r => r.json()).then(d => {
    if(d.custom) {
      document.getElementById('sent-status').innerText = 'Using Custom Sentences';
      document.getElementById('sent-text').value = d.text || '';
    } else {
      document.getElementById('sent-status').innerText = 'Using Factory Built-in';
    }
    if(d.quota) {
      const q = document.getElementById('sent-quota');
      q.innerText = d.quota;
      if (d.percent > 90) q.style.color = 'red';
    }
  }).catch(e => console.error(e));
}

document.addEventListener('DOMContentLoaded', () => {
  updateStatus();
  loadSentencesStatus();
  const tzInput = document.getElementById('dev-tz');
  if(tzInput) {
    tzInput.addEventListener('input', () => {
      const tzSelect = document.getElementById('tz-preset');
      if(tzSelect) tzSelect.value = '';
    });
  }
});

function scanWifi() {
  fetch('/api/wifi/scan').then(r => r.json()).then(d => {
    const s = document.getElementById('wifi-select');
    s.innerHTML = '<option value="">Select network...</option>';
    (d.networks || []).forEach(n => {
      s.innerHTML += `<option value="${n.ssid}">${n.ssid} (${n.rssi})</option>`;
    });
    showToast('Scan complete');
  }).catch(() => showToast('Scan failed'));
}

function saveWifi() {
  const ssid = document.getElementById('wifi-ssid').value;
  const pass = document.getElementById('wifi-pass').value;
  fetch('/api/wifi', {
    method: 'POST', headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({ssid, pass})
  }).then(r => r.ok ? showToast('WiFi saved!') : showToast('Error saving WiFi'));
}

function clearWifi() {
  if(!confirm('Clear WiFi credentials?')) return;
  fetch('/api/wifi/clear', {method: 'POST'})
    .then(r => r.ok ? showToast('WiFi cleared!') : showToast('Error clearing WiFi'));
}

function saveConfig(fields) {
  const body = {};
  fields.forEach(f => {
    const key = f.replace('dev-', '');
    body[key] = document.getElementById(f).value;
  });
  fetch('/api/config', {
    method: 'POST', headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(body)
  }).then(r => r.ok ? showToast('Config saved!') : showToast('Error saving config'));
}

function savePreferences() {
  const lang = document.querySelector('input[name="pref-lang"]:checked').value;
  const timer_mode = null; //parseInt(document.querySelector('input[name="pref-timer"]:checked').value, 10);
  const toVal = 0; //parseFloat(document.getElementById('pref-temp-offset').value);
  const temp_offset = Number.isNaN(toVal) ? 0.0 : toVal;
  const ntVal = parseInt(document.getElementById('pref-night-thresh').value, 10);
  const night_threshold = Number.isNaN(ntVal) ? 300 : ntVal;
  const stVal = parseFloat(document.getElementById('pref-slap-thresh').value);
  const slap_threshold = Number.isNaN(stVal) ? 2.0 : stVal;
  const invert_pot_l = document.getElementById('pref-inv-pot-l').checked;
  const invert_pot_r = document.getElementById('pref-inv-pot-r').checked;
  const invert_pwr = document.getElementById('pref-inv-pwr').checked;
  fetch('/api/config', {
    method: 'POST', headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({lang, timer_mode, temp_offset, night_threshold, slap_threshold, invert_pot_l, invert_pot_r, invert_pwr})
  }).then(r => r.ok ? showToast('Preferences saved!') : showToast('Error saving prefs'));
}

function saveOrientation() {
  fetch('/api/orientation/default', {method: 'POST'})
    .then(r => r.json())
    .then(d => {
      if(d.success) {
        showToast('Default orientation saved!');
        const txt = document.getElementById('pref-orient-text');
        if(txt) {
          txt.innerText = 'Calibrated';
          txt.style.color = '#44ff44';
        }
      } else {
        showToast('Error saving orientation');
      }
    })
    .catch(() => showToast('Error saving orientation'));
}

function saveSentences() {
  const text = document.getElementById('sent-text').value;
  fetch('/api/sentences', {
    method: 'POST', headers: {'Content-Type': 'text/plain'},
    body: text
  }).then(r => {
    if(r.ok) { showToast('Sentences saved!'); loadSentencesStatus(); }
    else showToast('Error saving sentences');
  });
}

function loadTemplate() {
  fetch('/api/sentences?source=builtin').then(r => r.text()).then(t => {
    document.getElementById('sent-text').value = t;
    showToast('Template loaded');
  });
}

function deleteSentences() {
  if(!confirm('Delete custom sentences and revert to built-in?')) return;
  fetch('/api/sentences', {method: 'DELETE'})
    .then(r => {
      if(r.ok) { showToast('Deleted custom sentences'); document.getElementById('sent-text').value=''; loadSentencesStatus(); }
      else showToast('Error deleting');
    });
}

function doOTA() {
  const fi = document.getElementById('ota-file');
  if(!fi.files.length) return showToast('Select a file first');
  const file = fi.files[0];
  const fd = new FormData(); fd.append('file', file);
  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/ota', true);
  document.getElementById('ota-progress-container').style.display = 'block';
  xhr.upload.onprogress = (e) => {
    if(e.lengthComputable) {
      const p = (e.loaded / e.total) * 100;
      document.getElementById('ota-progress').style.width = p + '%';
    }
  };
  xhr.onload = () => {
    if(xhr.status == 200) { showToast('Upload complete! Rebooting...'); setTimeout(() => location.reload(), 3000); }
    else showToast('Upload failed: ' + xhr.responseText);
  };
  xhr.send(fd);
}

function restartBot() {
  if(!confirm('Restart bot?')) return;
  fetch('/api/restart', {method: 'POST'}).then(() => showToast('Restarting...'));
}
</script>
</body>
</html>
)rawliteral";
} // namespace WebUi

#endif // FRIENDLYBOT_SERVICES_WEBUI_H
