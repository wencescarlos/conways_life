/*
 * Web Interface for Conway's Game of Life
 * Stored in PROGMEM to save RAM
 */

const char WEB_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>Conway's Game of Life</title>
<style>
:root {
  --bg: #0a0a0f;
  --panel: #12121a;
  --border: #1e1e2e;
  --accent: #00ff88;
  --accent2: #00ccff;
  --text: #e0e0e0;
  --dim: #666;
  --danger: #ff4444;
  --warn: #ffaa00;
}
* { margin:0; padding:0; box-sizing:border-box; }
body {
  font-family: 'Courier New', monospace;
  background: var(--bg);
  color: var(--text);
  overflow-x: hidden;
  min-height: 100vh;
}
.header {
  background: linear-gradient(135deg, #0d0d15 0%, #1a0a2e 100%);
  border-bottom: 1px solid var(--border);
  padding: 12px 20px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 8px;
}
.header h1 {
  font-size: 1.2em;
  color: var(--accent);
  text-shadow: 0 0 20px rgba(0,255,136,0.3);
}
.header .stats {
  font-size: 0.75em;
  color: var(--dim);
  display: flex;
  gap: 15px;
  flex-wrap: wrap;
}
.header .stats span { color: var(--accent2); }

.main-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 10px;
  gap: 10px;
}

/* Grid Canvas */
.grid-wrap {
  position: relative;
  border: 1px solid var(--border);
  border-radius: 8px;
  overflow: hidden;
  box-shadow: 0 0 30px rgba(0,255,136,0.05);
  background: #000;
  touch-action: none;
}
#gridCanvas {
  display: block;
  cursor: crosshair;
  image-rendering: pixelated;
}

/* Controls */
.controls {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  justify-content: center;
  max-width: 700px;
  width: 100%;
}
.btn {
  background: var(--panel);
  border: 1px solid var(--border);
  color: var(--text);
  padding: 8px 14px;
  border-radius: 6px;
  cursor: pointer;
  font-family: inherit;
  font-size: 0.8em;
  transition: all 0.2s;
  white-space: nowrap;
}
.btn:hover { border-color: var(--accent); color: var(--accent); }
.btn:active { transform: scale(0.95); }
.btn.active { background: var(--accent); color: var(--bg); border-color: var(--accent); }
.btn.danger { border-color: var(--danger); color: var(--danger); }
.btn.danger:hover { background: var(--danger); color: white; }
.btn.warn { border-color: var(--warn); color: var(--warn); }

/* Panels */
.panels {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  max-width: 700px;
  width: 100%;
  justify-content: center;
}
.panel {
  background: var(--panel);
  border: 1px solid var(--border);
  border-radius: 8px;
  padding: 12px;
  flex: 1;
  min-width: 200px;
}
.panel h3 {
  font-size: 0.85em;
  color: var(--accent);
  margin-bottom: 8px;
  border-bottom: 1px solid var(--border);
  padding-bottom: 4px;
}
.panel label {
  display: block;
  font-size: 0.75em;
  color: var(--dim);
  margin: 6px 0 3px;
}
.panel input, .panel select {
  width: 100%;
  background: var(--bg);
  border: 1px solid var(--border);
  color: var(--text);
  padding: 6px 8px;
  border-radius: 4px;
  font-family: inherit;
  font-size: 0.8em;
}
.panel input:focus, .panel select:focus {
  outline: none;
  border-color: var(--accent);
}
.slider-row {
  display: flex;
  align-items: center;
  gap: 8px;
}
.slider-row input[type="range"] {
  flex: 1;
  accent-color: var(--accent);
}
.slider-val {
  min-width: 45px;
  text-align: right;
  font-size: 0.8em;
  color: var(--accent);
}

/* Pattern buttons */
.pattern-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 4px;
}
.pattern-btn {
  background: var(--bg);
  border: 1px solid var(--border);
  color: var(--text);
  padding: 6px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 0.7em;
  font-family: inherit;
  transition: all 0.2s;
}
.pattern-btn:hover { border-color: var(--accent2); color: var(--accent2); }

/* Status bar */
.status-bar {
  display: flex;
  gap: 15px;
  flex-wrap: wrap;
  justify-content: center;
  font-size: 0.75em;
  color: var(--dim);
  padding: 6px;
}
.status-bar .alive { color: var(--accent); }
.status-bar .number { color: var(--accent2); }

/* WiFi section */
.wifi-networks {
  max-height: 120px;
  overflow-y: auto;
  margin: 6px 0;
}
.wifi-item {
  padding: 4px 8px;
  cursor: pointer;
  font-size: 0.75em;
  border-radius: 3px;
  display: flex;
  justify-content: space-between;
}
.wifi-item:hover { background: var(--bg); }
.wifi-item .rssi { color: var(--dim); }

/* Toast notifications */
.toast {
  position: fixed;
  bottom: 20px;
  right: 20px;
  background: var(--panel);
  border: 1px solid var(--accent);
  color: var(--accent);
  padding: 10px 20px;
  border-radius: 6px;
  font-size: 0.8em;
  opacity: 0;
  transition: opacity 0.3s;
  z-index: 100;
  pointer-events: none;
}
.toast.show { opacity: 1; }

/* Draw mode indicator */
.draw-indicator {
  position: fixed;
  top: 10px;
  right: 10px;
  background: var(--panel);
  border: 1px solid var(--accent);
  padding: 4px 10px;
  border-radius: 12px;
  font-size: 0.7em;
  display: none;
  z-index: 50;
}

/* Responsive */
@media (max-width: 500px) {
  .header { padding: 8px 12px; }
  .header h1 { font-size: 1em; }
  .btn { padding: 6px 10px; font-size: 0.7em; }
  .panel { min-width: 100%; }
}

/* Scrollbar */
::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: var(--bg); }
::-webkit-scrollbar-thumb { background: var(--border); border-radius: 3px; }
</style>
</head>
<body>

<div class="header">
  <h1>&#x2B22; Conway's Game of Life</h1>
  <div class="stats">
    Gen: <span id="hGen">0</span> |
    Pop: <span id="hPop">0</span> |
    Games: <span id="hGames">0</span> |
    <span id="hState" style="color:var(--accent)">RUNNING</span>
  </div>
</div>

<div class="main-container">
  <!-- Grid -->
  <div class="grid-wrap">
    <canvas id="gridCanvas"></canvas>
  </div>

  <!-- Controls -->
  <div class="controls">
    <button class="btn active" id="btnPause" onclick="togglePause()">&#9646;&#9646; Pause</button>
    <button class="btn" onclick="stepOnce()">&#9654;| Step</button>
    <button class="btn" onclick="randomGrid()">&#x1F3B2; Random</button>
    <button class="btn danger" onclick="clearGrid()">&#x2716; Clear</button>
    <button class="btn" id="btnDraw" onclick="toggleDrawMode()">&#x270F; Draw: ON</button>
    <button class="btn warn" onclick="toggleSection('patterns')">&#x2B22; Patterns</button>
    <button class="btn" onclick="toggleSection('settings')">&#x2699; Settings</button>
    <button class="btn" onclick="toggleSection('wifi')">&#x1F4F6; WiFi</button>
  </div>

  <!-- Status -->
  <div class="status-bar">
    <div>Generation: <span class="number" id="sGen">0</span></div>
    <div>Population: <span class="alive" id="sPop">0</span></div>
    <div>Max Pop: <span class="number" id="sMax">0</span></div>
    <div>Total Games: <span class="number" id="sGames">0</span></div>
    <div>Speed: <span class="number" id="sSpeed">150</span>ms</div>
    <div>Heap: <span class="number" id="sHeap">-</span></div>
  </div>

  <!-- Panels -->
  <div class="panels">
    <!-- Patterns Panel -->
    <div class="panel" id="panelPatterns" style="display:none">
      <h3>&#x2B22; Insert Pattern</h3>
      <p style="font-size:0.7em;color:var(--dim);margin-bottom:8px">
        Click to place at grid center, or click a grid position first.
      </p>
      <div class="pattern-grid">
        <button class="pattern-btn" onclick="insertPattern(0)">&#x2197; Glider</button>
        <button class="pattern-btn" onclick="insertPattern(1)">&#x1F680; LWSS</button>
        <button class="pattern-btn" onclick="insertPattern(2)">&#x2B50; Pulsar</button>
        <button class="pattern-btn" onclick="insertPattern(3)">&#x1F52B; Gosper Gun</button>
        <button class="pattern-btn" onclick="insertPattern(4)">&#x1F4A5; R-Pentomino</button>
        <button class="pattern-btn" onclick="insertPattern(5)">&#x1F330; Acorn</button>
        <button class="pattern-btn" onclick="insertPattern(6)">&#x1F480; Diehard</button>
        <button class="pattern-btn" onclick="insertPattern(7)">&#x1F3AF; Random Soup</button>
      </div>
    </div>

    <!-- Settings Panel -->
    <div class="panel" id="panelSettings" style="display:none">
      <h3>&#x2699; Settings</h3>
      <label>Simulation Speed</label>
      <div class="slider-row">
        <input type="range" id="speedSlider" min="20" max="1000" value="150" 
               oninput="updateSpeedLabel()" onchange="setSpeed()">
        <span class="slider-val" id="speedVal">150ms</span>
      </div>
      
      <label>Brightness</label>
      <div class="slider-row">
        <input type="range" id="brightSlider" min="0" max="100" value="100" 
               oninput="updateBrightLabel()" onchange="setBrightness()">
        <span class="slider-val" id="brightVal">100%</span>
      </div>
      
      <label>Color Theme</label>
      <select id="themeSelect" onchange="setTheme()"></select>
      
      <label>Screen Schedule</label>
      <div style="display:flex;align-items:center;gap:8px;margin:4px 0">
        <input type="checkbox" id="schedEnable" onchange="setSchedule()">
        <span style="font-size:0.75em">Enable auto on/off</span>
      </div>
      <div style="display:flex;gap:8px;align-items:center">
        <div>
          <label>On at</label>
          <input type="number" id="schedOn" min="0" max="23" value="7" 
                 style="width:60px" onchange="setSchedule()">
        </div>
        <div>
          <label>Off at</label>
          <input type="number" id="schedOff" min="0" max="23" value="23" 
                 style="width:60px" onchange="setSchedule()">
        </div>
      </div>
      
      <div style="margin-top:10px;display:flex;gap:6px">
        <button class="btn" onclick="toggleScreen(true)">&#x1F4A1; Screen ON</button>
        <button class="btn danger" onclick="toggleScreen(false)">Screen OFF</button>
      </div>
    </div>

    <!-- WiFi Panel -->
    <div class="panel" id="panelWifi" style="display:none">
      <h3>&#x1F4F6; WiFi Configuration</h3>
      <button class="btn" onclick="scanWifi()" style="margin-bottom:8px">Scan Networks</button>
      <div class="wifi-networks" id="wifiList"></div>
      <label>SSID</label>
      <input type="text" id="wifiSSID" placeholder="Network name">
      <label>Password</label>
      <input type="password" id="wifiPass" placeholder="Password">
      <label>Device Name</label>
      <input type="text" id="deviceName" placeholder="ConwayLife">
      <button class="btn active" onclick="saveWifi()" style="margin-top:8px;width:100%">
        Save &amp; Restart
      </button>
    </div>
  </div>
</div>

<div class="draw-indicator" id="drawIndicator">Drawing Mode</div>
<div class="toast" id="toast"></div>

<script>
// ─── Configuration ───
const CELL_PX = 5;       // Pixel size per cell on web
let COLS = 80, ROWS = 55;
let cells = new Uint8Array(COLS * ROWS);
let isRunning = true;
let drawMode = true;
let isDrawing = false;
let drawValue = 1;
let pollInterval;
let lastClickCol = -1, lastClickRow = -1;

// ─── Canvas Setup ───
const canvas = document.getElementById('gridCanvas');
const ctx = canvas.getContext('2d');

function resizeCanvas() {
  const maxW = Math.min(window.innerWidth - 30, 700);
  const cellSize = Math.max(2, Math.floor(maxW / COLS));
  canvas.width = COLS * cellSize;
  canvas.height = ROWS * cellSize;
  canvas.dataset.cellSize = cellSize;
  drawAllCells();
}

// ─── Color Theme (web side) ───
const webThemes = [
  {alive:'#00ff88', dead:'#0a0a0f', newborn:'#88ffcc'},
  {alive:'#00ccff', dead:'#000820', newborn:'#80e0ff'},
  {alive:'#ff3300', dead:'#1a0800', newborn:'#ff8800'},
  {alive:'#ff00ff', dead:'#0a000a', newborn:'#ffaaff'},
  {alive:'#ffee00', dead:'#18180a', newborn:'#ffffaa'},
  {alive:'#aaddff', dead:'#000518', newborn:'#ffffff'},
  {alive:'#ff8800', dead:'#1a0a00', newborn:'#ffcc00'},
  {alive:'#ffffff', dead:'#000000', newborn:'#cccccc'},
];
let currentWebTheme = 0;

// ─── Drawing ───
function drawAllCells() {
  const cs = parseInt(canvas.dataset.cellSize) || CELL_PX;
  const t = webThemes[currentWebTheme];
  ctx.fillStyle = t.dead;
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  
  // Draw grid lines (subtle)
  if (cs > 3) {
    ctx.strokeStyle = 'rgba(255,255,255,0.03)';
    ctx.lineWidth = 0.5;
    for (let x = 0; x <= COLS; x++) {
      ctx.beginPath(); ctx.moveTo(x*cs,0); ctx.lineTo(x*cs,ROWS*cs); ctx.stroke();
    }
    for (let y = 0; y <= ROWS; y++) {
      ctx.beginPath(); ctx.moveTo(0,y*cs); ctx.lineTo(COLS*cs,y*cs); ctx.stroke();
    }
  }
  
  // Draw alive cells
  for (let r = 0; r < ROWS; r++) {
    for (let c = 0; c < COLS; c++) {
      if (cells[r * COLS + c]) {
        ctx.fillStyle = t.alive;
        ctx.fillRect(c*cs+1, r*cs+1, cs-1, cs-1);
      }
    }
  }
}

function drawCell(col, row, alive) {
  const cs = parseInt(canvas.dataset.cellSize) || CELL_PX;
  const t = webThemes[currentWebTheme];
  ctx.fillStyle = alive ? t.newborn : t.dead;
  ctx.fillRect(col*cs + (alive?1:0), row*cs + (alive?1:0), cs - (alive?1:0), cs);
}

// ─── Canvas Events ───
canvas.addEventListener('mousedown', (e) => { if(drawMode) startDraw(e); });
canvas.addEventListener('mousemove', (e) => { if(isDrawing) continueDraw(e); });
canvas.addEventListener('mouseup', () => stopDraw());
canvas.addEventListener('mouseleave', () => stopDraw());
canvas.addEventListener('touchstart', (e) => { 
  e.preventDefault(); if(drawMode) startDraw(e.touches[0]); 
}, {passive:false});
canvas.addEventListener('touchmove', (e) => { 
  e.preventDefault(); if(isDrawing) continueDraw(e.touches[0]); 
}, {passive:false});
canvas.addEventListener('touchend', () => stopDraw());

function getCellFromEvent(e) {
  const rect = canvas.getBoundingClientRect();
  const cs = parseInt(canvas.dataset.cellSize) || CELL_PX;
  const x = (e.clientX || e.pageX) - rect.left;
  const y = (e.clientY || e.pageY) - rect.top;
  return { col: Math.floor(x/cs), row: Math.floor(y/cs) };
}

function startDraw(e) {
  isDrawing = true;
  const {col, row} = getCellFromEvent(e);
  if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return;
  lastClickCol = col; lastClickRow = row;
  drawValue = cells[row*COLS+col] ? 0 : 1;
  cells[row*COLS+col] = drawValue;
  drawCell(col, row, drawValue);
  sendCell(col, row, drawValue);
}

function continueDraw(e) {
  const {col, row} = getCellFromEvent(e);
  if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return;
  if (col === lastClickCol && row === lastClickRow) return;
  lastClickCol = col; lastClickRow = row;
  cells[row*COLS+col] = drawValue;
  drawCell(col, row, drawValue);
  sendCell(col, row, drawValue);
}

function stopDraw() { isDrawing = false; }

// ─── API Calls ───
async function api(endpoint, method='GET', body=null) {
  try {
    const opts = { method };
    if (body) {
      opts.headers = {'Content-Type':'application/json'};
      opts.body = JSON.stringify(body);
    }
    const r = await fetch('/api/'+endpoint, opts);
    return await r.json();
  } catch(e) {
    console.error('API error:', e);
    return null;
  }
}

async function sendCell(col, row, alive) {
  api('cell', 'POST', {col, row, alive: alive===1});
}

async function fetchGrid() {
  const data = await api('grid');
  if (!data) return;
  COLS = data.cols; ROWS = data.rows;
  cells = new Uint8Array(COLS * ROWS);
  for (const [c, r] of data.cells) {
    cells[r * COLS + c] = 1;
  }
  updateStatsDisplay(data);
  drawAllCells();
}

async function fetchStats() {
  const data = await api('stats');
  if (!data) return;
  updateStatsDisplay(data);
}

function updateStatsDisplay(d) {
  if (d.gen !== undefined || d.generation !== undefined) {
    const gen = d.gen || d.generation || 0;
    const pop = d.pop || d.population || 0;
    document.getElementById('hGen').textContent = gen;
    document.getElementById('hPop').textContent = pop;
    document.getElementById('sGen').textContent = gen;
    document.getElementById('sPop').textContent = pop;
  }
  if (d.games !== undefined || d.totalGames !== undefined) {
    const g = d.games || d.totalGames || 0;
    document.getElementById('hGames').textContent = g;
    document.getElementById('sGames').textContent = g;
  }
  if (d.maxPopulation !== undefined) {
    document.getElementById('sMax').textContent = d.maxPopulation;
  }
  if (d.speed !== undefined) {
    document.getElementById('sSpeed').textContent = d.speed;
  }
  if (d.freeHeap !== undefined) {
    document.getElementById('sHeap').textContent = (d.freeHeap/1024).toFixed(1)+'KB';
  }
  if (d.running !== undefined) {
    isRunning = d.running;
    document.getElementById('hState').textContent = isRunning ? 'RUNNING' : 'PAUSED';
    document.getElementById('hState').style.color = isRunning ? 'var(--accent)' : 'var(--warn)';
    updatePauseBtn();
  }
}

// ─── Controls ───
async function togglePause() {
  if (isRunning) {
    await api('pause', 'POST');
    isRunning = false;
  } else {
    await api('resume', 'POST');
    isRunning = true;
  }
  updatePauseBtn();
  showToast(isRunning ? 'Simulation resumed' : 'Simulation paused');
}

function updatePauseBtn() {
  const btn = document.getElementById('btnPause');
  btn.textContent = isRunning ? '\u23F8 Pause' : '\u25B6 Resume';
  btn.classList.toggle('active', isRunning);
}

async function stepOnce() {
  if (isRunning) await togglePause();
  await api('step', 'POST');
  setTimeout(fetchGrid, 100);
}

async function randomGrid() {
  await api('random', 'POST');
  setTimeout(fetchGrid, 200);
  showToast('Random grid generated');
}

async function clearGrid() {
  await api('clear', 'POST');
  cells.fill(0);
  drawAllCells();
  showToast('Grid cleared');
}

function toggleDrawMode() {
  drawMode = !drawMode;
  const btn = document.getElementById('btnDraw');
  btn.textContent = drawMode ? '\u270F Draw: ON' : '\u270F Draw: OFF';
  btn.classList.toggle('active', drawMode);
  document.getElementById('drawIndicator').style.display = drawMode ? 'block' : 'none';
}

// ─── Patterns ───
async function insertPattern(type) {
  if (type === 7) {
    await randomGrid();
    return;
  }
  const col = lastClickCol >= 0 ? lastClickCol : Math.floor(COLS/2);
  const row = lastClickRow >= 0 ? lastClickRow : Math.floor(ROWS/2);
  await api('pattern', 'POST', {type, col, row});
  setTimeout(fetchGrid, 200);
  const names = ['Glider','LWSS','Pulsar','Gosper Gun','R-Pentomino','Acorn','Diehard'];
  showToast(names[type] + ' placed!');
}

// ─── Settings ───
function updateSpeedLabel() {
  document.getElementById('speedVal').textContent = 
    document.getElementById('speedSlider').value + 'ms';
}

function updateBrightLabel() {
  document.getElementById('brightVal').textContent = 
    document.getElementById('brightSlider').value + '%';
}

async function setSpeed() {
  const speed = parseInt(document.getElementById('speedSlider').value);
  await api('speed', 'POST', {speed});
  showToast('Speed: ' + speed + 'ms');
}

async function setBrightness() {
  const b = parseInt(document.getElementById('brightSlider').value);
  await api('brightness', 'POST', {brightness: b});
  showToast('Brightness: ' + b + '%');
}

async function setTheme() {
  const theme = parseInt(document.getElementById('themeSelect').value);
  currentWebTheme = theme;
  await api('theme', 'POST', {theme});
  drawAllCells();
  showToast('Theme changed');
}

async function setSchedule() {
  const enabled = document.getElementById('schedEnable').checked;
  const on = parseInt(document.getElementById('schedOn').value);
  const off = parseInt(document.getElementById('schedOff').value);
  await api('schedule', 'POST', {enabled, on, off});
  showToast('Schedule ' + (enabled ? 'enabled' : 'disabled'));
}

async function toggleScreen(on) {
  await api('screen', 'POST', {on});
  showToast('Screen ' + (on ? 'ON' : 'OFF'));
}

// ─── WiFi ───
async function scanWifi() {
  showToast('Scanning...');
  const networks = await api('wifi/scan');
  const list = document.getElementById('wifiList');
  list.innerHTML = '';
  if (!networks || networks.length === 0) {
    list.innerHTML = '<div style="font-size:0.75em;color:var(--dim)">No networks found</div>';
    return;
  }
  networks.sort((a,b) => b.rssi - a.rssi);
  for (const n of networks) {
    const div = document.createElement('div');
    div.className = 'wifi-item';
    div.innerHTML = `<span>${n.enc?'&#x1F512;':''} ${n.ssid}</span><span class="rssi">${n.rssi}dBm</span>`;
    div.onclick = () => { document.getElementById('wifiSSID').value = n.ssid; };
    list.appendChild(div);
  }
}

async function saveWifi() {
  const ssid = document.getElementById('wifiSSID').value;
  const pass = document.getElementById('wifiPass').value;
  const name = document.getElementById('deviceName').value;
  if (!ssid) { showToast('Enter SSID'); return; }
  if (!confirm('Device will restart with new WiFi settings. Continue?')) return;
  await api('wifi', 'POST', {ssid, pass, name});
  showToast('Restarting...');
}

// ─── Sections Toggle ───
function toggleSection(name) {
  const panels = ['patterns','settings','wifi'];
  for (const p of panels) {
    const el = document.getElementById('panel' + p.charAt(0).toUpperCase() + p.slice(1));
    if (p === name) {
      el.style.display = el.style.display === 'none' ? 'block' : 'none';
    } else {
      el.style.display = 'none';
    }
  }
}

// ─── Toast ───
function showToast(msg) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 2000);
}

// ─── Init ───
async function init() {
  // Load config
  const config = await api('config');
  if (config) {
    document.getElementById('speedSlider').value = config.speed;
    updateSpeedLabel();
    currentWebTheme = config.theme;
    
    // Populate theme selector
    const sel = document.getElementById('themeSelect');
    sel.innerHTML = '';
    if (config.themes) {
      config.themes.forEach((name, i) => {
        const opt = document.createElement('option');
        opt.value = i; opt.textContent = name;
        if (i === config.theme) opt.selected = true;
        sel.appendChild(opt);
      });
    }
    
    document.getElementById('schedEnable').checked = config.scheduleEnabled;
    document.getElementById('schedOn').value = config.scheduleOn;
    document.getElementById('schedOff').value = config.scheduleOff;
    if (config.brightness !== undefined) {
      document.getElementById('brightSlider').value = config.brightness;
      updateBrightLabel();
    }
    if (config.deviceName) {
      document.getElementById('deviceName').value = config.deviceName;
    }
  }
  
  // Load grid
  await fetchGrid();
  resizeCanvas();
  
  // Start polling
  pollInterval = setInterval(async () => {
    if (isRunning) {
      await fetchGrid();
    } else {
      await fetchStats();
    }
  }, 1000);
}

window.addEventListener('resize', resizeCanvas);
init();
</script>
</body>
</html>
)rawliteral";
