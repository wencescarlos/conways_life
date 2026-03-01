/*
 * Interfaz Web para El Juego de la Vida de Conway
 * Almacenado en PROGMEM para ahorrar RAM
 */

const char WEB_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>El Juego de la Vida de Conway</title>
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

/* Lienzo de la cuadrícula */
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

/* Controles */
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

/* Paneles */
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

/* Botones de patrones */
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

/* Barra de estado */
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

/* Sección WiFi */
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

/* Notificaciones emergentes */
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

/* Indicador de modo dibujo */
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

/* Adaptable */
@media (max-width: 500px) {
  .header { padding: 8px 12px; }
  .header h1 { font-size: 1em; }
  .btn { padding: 6px 10px; font-size: 0.7em; }
  .panel { min-width: 100%; }
}

/* Barra de desplazamiento */
::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: var(--bg); }
::-webkit-scrollbar-thumb { background: var(--border); border-radius: 3px; }
</style>
</head>
<body>

<div class="header">
  <h1>&#x2B22; El Juego de la Vida de Conway</h1>
  <div class="stats">
    Gen: <span id="hGen">0</span> |
    Pob: <span id="hPop">0</span> |
    Partidas: <span id="hGames">0</span> |
    <span id="hState" style="color:var(--accent)">EN EJECUCION</span>
  </div>
</div>

<div class="main-container">
  <!-- Cuadrícula -->
  <div class="grid-wrap">
    <canvas id="gridCanvas"></canvas>
  </div>

  <!-- Controles -->
  <div class="controls">
    <button class="btn active" id="btnPause" onclick="togglePause()">&#9646;&#9646; Pausa</button>
    <button class="btn" onclick="stepOnce()">&#9654;| Paso</button>
    <button class="btn" onclick="randomGrid()">&#x1F3B2; Aleatorio</button>
    <button class="btn danger" onclick="clearGrid()">&#x2716; Limpiar</button>
    <button class="btn" id="btnDraw" onclick="toggleDrawMode()">&#x270F; Dibujar: SI</button>
    <button class="btn warn" onclick="toggleSection('patterns')">&#x2B22; Patrones</button>
    <button class="btn" onclick="toggleSection('settings')">&#x2699; Ajustes</button>
    <button class="btn" onclick="toggleSection('wifi')">&#x1F4F6; WiFi</button>
    <button class="btn" onclick="toggleSection('firmware')">&#x1F504; Firmware</button>
    <button class="btn" onclick="toggleSection('popgraph')">&#x1F4C8; Poblaci&oacute;n</button>
    <button class="btn" onclick="toggleSection('custom')">&#x1F4BE; Mis Patrones</button>
    <button class="btn" onclick="toggleSection('rle')">&#x1F4CB; RLE</button>
  </div>

  <!-- Estado -->
  <div class="status-bar">
    <div>Generaci&oacute;n: <span class="number" id="sGen">0</span></div>
    <div>Poblaci&oacute;n: <span class="alive" id="sPop">0</span></div>
    <div>Pob. M&aacute;x: <span class="number" id="sMax">0</span></div>
    <div>Partidas: <span class="number" id="sGames">0</span></div>
    <div>Velocidad: <span class="number" id="sSpeed">150</span>ms</div>
    <div>Heap: <span class="number" id="sHeap">-</span></div>
  </div>

  <!-- Paneles -->
  <div class="panels">
    <!-- Panel de Patrones -->
    <div class="panel" id="panelPatterns" style="display:none">
      <h3>&#x2B22; Insertar Patr&oacute;n</h3>
      <p style="font-size:0.7em;color:var(--dim);margin-bottom:8px">
        Haz clic para colocar en el centro, o selecciona primero una posici&oacute;n en la cuadr&iacute;cula.
      </p>
      <div class="pattern-grid">
        <button class="pattern-btn" onclick="insertPattern(0)">&#x2197; Planeador</button>
        <button class="pattern-btn" onclick="insertPattern(1)">&#x1F680; LWSS (Nave)</button>
        <button class="pattern-btn" onclick="insertPattern(2)">&#x2B50; Pulsar</button>
        <button class="pattern-btn" onclick="insertPattern(3)">&#x1F52B; Ca&ntilde;&oacute;n Gosper</button>
        <button class="pattern-btn" onclick="insertPattern(4)">&#x1F4A5; R-Pentomino</button>
        <button class="pattern-btn" onclick="insertPattern(5)">&#x1F330; Bellota</button>
        <button class="pattern-btn" onclick="insertPattern(6)">&#x1F480; Diehard</button>
        <button class="pattern-btn" onclick="insertPattern(7)">&#x1F3AF; Sopa Aleatoria</button>
      </div>
    </div>

    <!-- Panel de Ajustes -->
    <div class="panel" id="panelSettings" style="display:none">
      <h3>&#x2699; Ajustes</h3>
      <label>Velocidad de Simulaci&oacute;n</label>
      <div class="slider-row">
        <input type="range" id="speedSlider" min="20" max="1000" value="150" 
               oninput="updateSpeedLabel()" onchange="setSpeed()">
        <span class="slider-val" id="speedVal">150ms</span>
      </div>
      
      <label>Brillo</label>
      <div class="slider-row">
        <input type="range" id="brightSlider" min="0" max="100" value="100" 
               oninput="updateBrightLabel()" onchange="setBrightness()">
        <span class="slider-val" id="brightVal">100%</span>
      </div>
      
      <label>Tema de Color</label>
      <select id="themeSelect" onchange="setTheme()"></select>
      
      <label>Programaci&oacute;n de Pantalla</label>
      <div style="display:flex;align-items:center;gap:8px;margin:4px 0">
        <input type="checkbox" id="schedEnable" onchange="setSchedule()">
        <span style="font-size:0.75em">Activar encendido/apagado autom&aacute;tico</span>
      </div>
      <div style="display:flex;gap:8px;align-items:center">
        <div>
          <label>Encender a las</label>
          <input type="number" id="schedOn" min="0" max="23" value="7" 
                 style="width:60px" onchange="setSchedule()">
        </div>
        <div>
          <label>Apagar a las</label>
          <input type="number" id="schedOff" min="0" max="23" value="23" 
                 style="width:60px" onchange="setSchedule()">
        </div>
      </div>
      
      <div style="margin-top:10px;display:flex;gap:6px">
        <button class="btn" onclick="toggleScreen(true)">&#x1F4A1; Pantalla ON</button>
        <button class="btn danger" onclick="toggleScreen(false)">Pantalla OFF</button>
      </div>
    </div>

    <!-- Panel WiFi -->
    <div class="panel" id="panelWifi" style="display:none">
      <h3>&#x1F4F6; Configuraci&oacute;n WiFi</h3>
      <button class="btn" onclick="scanWifi()" style="margin-bottom:8px">Buscar Redes</button>
      <div class="wifi-networks" id="wifiList"></div>
      <label>SSID</label>
      <input type="text" id="wifiSSID" placeholder="Nombre de la red">
      <label>Contrase&ntilde;a</label>
      <input type="password" id="wifiPass" placeholder="Contrase&ntilde;a">
      <label>Nombre del Dispositivo</label>
      <input type="text" id="deviceName" placeholder="ConwayLife">
      <button class="btn active" onclick="saveWifi()" style="margin-top:8px;width:100%">
        Guardar y Reiniciar
      </button>
    </div>

    <!-- Panel Firmware -->
    <div class="panel" id="panelFirmware" style="display:none">
      <h3>&#x1F504; Firmware y Actualizaci&oacute;n OTA</h3>
      <div style="font-size:0.8em;margin-bottom:10px">
        <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid var(--border)">
          <span style="color:var(--dim)">Versi&oacute;n:</span>
          <span id="fwVersion" style="color:var(--accent)">-</span>
        </div>
        <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid var(--border)">
          <span style="color:var(--dim)">Autor:</span>
          <span id="fwAuthor" style="color:var(--text)">-</span>
        </div>
        <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid var(--border)">
          <span style="color:var(--dim)">Chip ID:</span>
          <span id="fwChip" style="color:var(--text)">-</span>
        </div>
        <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid var(--border)">
          <span style="color:var(--dim)">Flash usado:</span>
          <span id="fwFlash" style="color:var(--text)">-</span>
        </div>
        <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid var(--border)">
          <span style="color:var(--dim)">SDK:</span>
          <span id="fwSdk" style="color:var(--text)">-</span>
        </div>
        <div style="padding:6px 0">
          <span style="color:var(--dim)">GitHub:</span>
          <a href="https://github.com/wencescarlos/conways_life" target="_blank" 
             style="color:var(--accent2);text-decoration:none;margin-left:6px">
            &#x1F517; wencescarlos/conways_life
          </a>
        </div>
      </div>
      
      <div style="background:var(--bg);border:1px solid var(--border);border-radius:6px;padding:10px;margin-bottom:8px">
        <p style="font-size:0.75em;color:var(--warn);margin-bottom:8px">
          &#x26A0; Subir un archivo .bin compilado. El dispositivo se reiniciar&aacute; autom&aacute;ticamente tras la actualizaci&oacute;n.
        </p>
        <p style="font-size:0.7em;color:var(--dim);margin-bottom:6px">
          &#x1F512; OTA protegida &mdash; Usuario: <span style="color:var(--accent2)">admin</span> / Clave: <span style="color:var(--accent2)">conway1234</span>
        </p>
        <div style="font-size:0.75em;color:var(--dim);margin-bottom:6px">
          Selecciona el archivo de firmware (.bin):
        </div>
        <input type="file" id="fwFile" accept=".bin" 
               style="width:100%;font-size:0.75em;color:var(--text);margin-bottom:8px">
        <div style="display:flex;gap:6px">
          <button class="btn active" onclick="uploadFirmware()" style="flex:1">
            &#x1F4E4; Actualizar Firmware
          </button>
        </div>
        <div id="fwProgress" style="display:none;margin-top:8px">
          <div style="background:var(--border);border-radius:3px;height:8px;overflow:hidden">
            <div id="fwBar" style="background:var(--accent);height:100%;width:0%;transition:width 0.3s"></div>
          </div>
          <div id="fwStatus" style="font-size:0.7em;color:var(--dim);margin-top:4px;text-align:center">
            Subiendo...
          </div>
        </div>
      </div>
    </div>

    <!-- Panel Gráfico de Población -->
    <div class="panel" id="panelPopgraph" style="display:none;min-width:100%">
      <h3>&#x1F4C8; Gr&aacute;fico de Poblaci&oacute;n</h3>
      <canvas id="popCanvas" style="width:100%;height:180px;background:#000;border-radius:4px;border:1px solid var(--border)"></canvas>
      <div style="display:flex;justify-content:space-between;font-size:0.7em;color:var(--dim);margin-top:4px">
        <span>Poblaci&oacute;n actual: <span style="color:var(--accent)" id="popCurrent">0</span></span>
        <span>M&aacute;ximo: <span style="color:var(--accent2)" id="popMax">0</span></span>
        <span id="wsStatus" style="color:var(--dim)">&#x25CF; WS</span>
      </div>
    </div>

    <!-- Panel Mis Patrones -->
    <div class="panel" id="panelCustom" style="display:none">
      <h3>&#x1F4BE; Mis Patrones Guardados</h3>
      <div style="display:flex;gap:6px;margin-bottom:8px">
        <input type="text" id="patternName" placeholder="Nombre del patr&oacute;n" style="flex:1">
        <button class="btn active" onclick="savePattern()">Guardar</button>
      </div>
      <div id="customPatternList" style="font-size:0.8em;max-height:150px;overflow-y:auto"></div>
      <button class="btn" onclick="loadCustomPatterns()" style="margin-top:6px;width:100%">&#x1F504; Refrescar lista</button>
    </div>

    <!-- Panel RLE Import/Export -->
    <div class="panel" id="panelRle" style="display:none;min-width:100%">
      <h3>&#x1F4CB; Importar / Exportar RLE</h3>
      <p style="font-size:0.7em;color:var(--dim);margin-bottom:6px">
        El formato RLE es el est&aacute;ndar de la comunidad para compartir patrones del Juego de la Vida.
      </p>
      <div style="display:flex;gap:6px;margin-bottom:8px">
        <button class="btn active" onclick="exportRLE()" style="flex:1">&#x1F4E4; Exportar</button>
        <button class="btn" onclick="importRLE()" style="flex:1">&#x1F4E5; Importar</button>
      </div>
      <textarea id="rleData" rows="8" placeholder="Pega aqu&iacute; un patr&oacute;n RLE para importar, o pulsa Exportar para obtener el actual..." 
                style="width:100%;background:var(--bg);border:1px solid var(--border);color:var(--text);padding:8px;border-radius:4px;font-family:monospace;font-size:0.75em;resize:vertical"></textarea>
      <div style="font-size:0.65em;color:var(--dim);margin-top:4px">
        Ejemplo: <code>bo$2bo$3o!</code> (Planeador) &mdash; 
        <a href="https://conwaylife.com/wiki/Run_Length_Encoded" target="_blank" style="color:var(--accent2)">Info RLE</a>
      </div>
    </div>
  </div>

  <!-- Pie de p&aacute;gina -->
  <div style="text-align:center;padding:15px 10px;font-size:0.65em;color:#444;border-top:1px solid var(--border);margin-top:10px">
    <div>&#x2B22; Conway's Life ESP8266 &mdash; Firmware v<span id="footerVersion">2.0</span></div>
    <div style="margin-top:3px">
      Desarrollado por <span style="color:var(--accent2)">wencescarlos</span> &mdash;
      <a href="https://github.com/wencescarlos/conways_life" target="_blank" 
         style="color:var(--accent);text-decoration:none">
        &#x1F517; GitHub
      </a>
    </div>
  </div>
</div>

<div class="draw-indicator" id="drawIndicator">Modo Dibujo</div>
<div class="toast" id="toast"></div>

<script>
// ─── Configuración ───
const CELL_PX = 5;       // Tamaño en píxeles por célula en la web
let COLS = 80, ROWS = 55;
let cells = new Uint8Array(COLS * ROWS);
let isRunning = true;
let drawMode = true;
let isDrawing = false;
let drawValue = 1;
let pollInterval;
let lastClickCol = -1, lastClickRow = -1;

// ─── Configuración del Lienzo ───
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

// ─── Tema de Color (lado web) ───
const webThemes = [
  // Clásicos
  {alive:'#00ff88', dead:'#0a0a0f', newborn:'#88ffcc'},   // Matrix
  {alive:'#ffffff', dead:'#000000', newborn:'#cccccc'},   // Monocromo
  {alive:'#ffaa00', dead:'#1a0800', newborn:'#ffdd00'},   // Ambar
  {alive:'#44ff44', dead:'#000800', newborn:'#88ff88'},   // Fosforo
  // Naturaleza
  {alive:'#00ccff', dead:'#000820', newborn:'#80e0ff'},   // Oceano
  {alive:'#22cc44', dead:'#021000', newborn:'#88ee88'},   // Bosque
  {alive:'#ff8800', dead:'#1a0a00', newborn:'#ffcc00'},   // Atardecer
  {alive:'#00ffaa', dead:'#000820', newborn:'#aaffee'},   // Aurora
  {alive:'#ff7799', dead:'#180808', newborn:'#ffaacc'},   // Coral
  // Energía
  {alive:'#ff3300', dead:'#1a0800', newborn:'#ff8800'},   // Lava
  {alive:'#ff00ff', dead:'#0a000a', newborn:'#ffaaff'},   // Neon
  {alive:'#0044ff', dead:'#000008', newborn:'#8888ff'},   // Electrico
  {alive:'#ee00ee', dead:'#100010', newborn:'#ff88ff'},   // Plasma
  {alive:'#ffee00', dead:'#18180a', newborn:'#ffffaa'},   // Solar
  // Ambientes
  {alive:'#aaddff', dead:'#000518', newborn:'#ffffff'},   // Hielo
  {alive:'#7744ff', dead:'#000008', newborn:'#bb88ff'},   // Medianoche
  {alive:'#ddaa66', dead:'#221a0a', newborn:'#eecc88'},   // Desierto
  {alive:'#cc0088', dead:'#080010', newborn:'#ff44bb'},   // Crepusculo
  // Especiales
  {alive:'#ffee00', dead:'#181818', newborn:'#00ccff'},   // Retro
  {alive:'#004488', dead:'#000410', newborn:'#0066aa'},   // Cianotipia
];
let currentWebTheme = 0;

// ─── Dibujado ───
function drawAllCells() {
  const cs = parseInt(canvas.dataset.cellSize) || CELL_PX;
  const t = webThemes[currentWebTheme];
  ctx.fillStyle = t.dead;
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  
  // Dibujar líneas de cuadrícula (sutiles)
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
  
  // Dibujar células vivas
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

// ─── Eventos del Lienzo ───
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

// ─── Llamadas API ───
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
    console.error('Error API:', e);
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
    document.getElementById('hState').textContent = isRunning ? 'EN EJECUCION' : 'EN PAUSA';
    document.getElementById('hState').style.color = isRunning ? 'var(--accent)' : 'var(--warn)';
    updatePauseBtn();
  }
}

// ─── Controles ───
async function togglePause() {
  if (isRunning) {
    await api('pause', 'POST');
    isRunning = false;
  } else {
    await api('resume', 'POST');
    isRunning = true;
  }
  updatePauseBtn();
  showToast(isRunning ? 'Simulacion reanudada' : 'Simulacion pausada');
}

function updatePauseBtn() {
  const btn = document.getElementById('btnPause');
  btn.textContent = isRunning ? '\u23F8 Pausa' : '\u25B6 Reanudar';
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
  showToast('Cuadricula aleatoria generada');
}

async function clearGrid() {
  await api('clear', 'POST');
  cells.fill(0);
  drawAllCells();
  showToast('Cuadricula limpiada');
}

function toggleDrawMode() {
  drawMode = !drawMode;
  const btn = document.getElementById('btnDraw');
  btn.textContent = drawMode ? '\u270F Dibujar: SI' : '\u270F Dibujar: NO';
  btn.classList.toggle('active', drawMode);
  document.getElementById('drawIndicator').style.display = drawMode ? 'block' : 'none';
}

// ─── Patrones ───
async function insertPattern(type) {
  if (type === 7) {
    await randomGrid();
    return;
  }
  const col = lastClickCol >= 0 ? lastClickCol : Math.floor(COLS/2);
  const row = lastClickRow >= 0 ? lastClickRow : Math.floor(ROWS/2);
  await api('pattern', 'POST', {type, col, row});
  setTimeout(fetchGrid, 200);
  const names = ['Planeador','LWSS','Pulsar','Canon Gosper','R-Pentomino','Bellota','Diehard'];
  showToast(names[type] + ' colocado!');
}

// ─── Ajustes ───
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
  showToast('Velocidad: ' + speed + 'ms');
}

async function setBrightness() {
  const b = parseInt(document.getElementById('brightSlider').value);
  await api('brightness', 'POST', {brightness: b});
  showToast('Brillo: ' + b + '%');
}

async function setTheme() {
  const theme = parseInt(document.getElementById('themeSelect').value);
  currentWebTheme = theme;
  await api('theme', 'POST', {theme});
  drawAllCells();
  showToast('Tema cambiado');
}

async function setSchedule() {
  const enabled = document.getElementById('schedEnable').checked;
  const on = parseInt(document.getElementById('schedOn').value);
  const off = parseInt(document.getElementById('schedOff').value);
  await api('schedule', 'POST', {enabled, on, off});
  showToast('Programacion ' + (enabled ? 'activada' : 'desactivada'));
}

async function toggleScreen(on) {
  await api('screen', 'POST', {on});
  showToast('Pantalla ' + (on ? 'ENCENDIDA' : 'APAGADA'));
}

// ─── WiFi ───
async function scanWifi() {
  showToast('Buscando...');
  const networks = await api('wifi/scan');
  const list = document.getElementById('wifiList');
  list.innerHTML = '';
  if (!networks || networks.length === 0) {
    list.innerHTML = '<div style="font-size:0.75em;color:var(--dim)">No se encontraron redes</div>';
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
  if (!ssid) { showToast('Introduce el SSID'); return; }
  if (!confirm('El dispositivo se reiniciara con los nuevos ajustes WiFi. Continuar?')) return;
  await api('wifi', 'POST', {ssid, pass, name});
  showToast('Reiniciando...');
}

// ─── Alternar Secciones ───
function toggleSection(name) {
  const panels = ['patterns','settings','wifi','firmware','popgraph','custom','rle'];
  for (const p of panels) {
    const el = document.getElementById('panel' + p.charAt(0).toUpperCase() + p.slice(1));
    if (p === name) {
      el.style.display = el.style.display === 'none' ? 'block' : 'none';
      if (p === 'firmware' && el.style.display === 'block') fetchFirmwareInfo();
      if (p === 'popgraph' && el.style.display === 'block') { fetchPopHistory(); drawPopGraph(); }
      if (p === 'custom' && el.style.display === 'block') loadCustomPatterns();
    } else {
      el.style.display = 'none';
    }
  }
}

// ─── Notificación ───
function showToast(msg) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 2000);
}

// ─── Firmware ───
async function fetchFirmwareInfo() {
  const data = await api('firmware');
  if (!data) return;
  document.getElementById('fwVersion').textContent = 'v' + data.version;
  document.getElementById('fwAuthor').textContent = data.author;
  document.getElementById('fwChip').textContent = data.chipId;
  document.getElementById('fwSdk').textContent = data.sdkVersion;
  const usedKB = (data.sketchSize / 1024).toFixed(0);
  const freeKB = (data.freeSketch / 1024).toFixed(0);
  document.getElementById('fwFlash').textContent = usedKB + 'KB / ' + freeKB + 'KB libre';
  document.getElementById('footerVersion').textContent = data.version;
}

async function uploadFirmware() {
  const fileInput = document.getElementById('fwFile');
  if (!fileInput.files.length) { showToast('Selecciona un archivo .bin'); return; }
  const file = fileInput.files[0];
  if (!file.name.endsWith('.bin')) { showToast('Solo archivos .bin'); return; }
  if (!confirm('Se actualizara el firmware y el dispositivo se reiniciara. Continuar?')) return;
  const progressDiv = document.getElementById('fwProgress');
  const bar = document.getElementById('fwBar');
  const status = document.getElementById('fwStatus');
  progressDiv.style.display = 'block';
  bar.style.width = '0%';
  status.textContent = 'Subiendo firmware...';
  const formData = new FormData();
  formData.append('update', file);
  try {
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/update', true);
    // Autenticación OTA (usuario: admin, clave: conway1234)
    xhr.setRequestHeader('Authorization', 'Basic ' + btoa('admin:conway1234'));
    xhr.upload.onprogress = function(e) {
      if (e.lengthComputable) {
        const pct = Math.round((e.loaded / e.total) * 100);
        bar.style.width = pct + '%';
        status.textContent = 'Subiendo... ' + pct + '%';
      }
    };
    xhr.onload = function() {
      if (xhr.status === 200) {
        bar.style.width = '100%';
        bar.style.background = 'var(--accent)';
        status.textContent = 'Firmware actualizado! Reiniciando...';
        showToast('Firmware actualizado correctamente');
        setTimeout(() => { location.reload(); }, 10000);
      } else {
        bar.style.background = 'var(--danger)';
        status.textContent = 'Error: ' + xhr.statusText;
        showToast('Error al actualizar');
      }
    };
    xhr.onerror = function() {
      bar.style.background = 'var(--danger)';
      status.textContent = 'Error de conexion';
    };
    xhr.send(formData);
  } catch(e) {
    status.textContent = 'Error: ' + e.message;
  }
}

// ─── WebSocket Tiempo Real ───
let ws = null;
let popData = [];
const MAX_POP_POINTS = 200;

function connectWebSocket() {
  const host = location.hostname;
  ws = new WebSocket('ws://' + host + ':81');
  ws.onopen = function() {
    const el = document.getElementById('wsStatus');
    if (el) { el.style.color = 'var(--accent)'; el.textContent = '\\u25CF WS Conectado'; }
  };
  ws.onclose = function() {
    const el = document.getElementById('wsStatus');
    if (el) { el.style.color = 'var(--danger)'; el.textContent = '\\u25CF WS Desconectado'; }
    setTimeout(connectWebSocket, 3000);
  };
  ws.onmessage = function(evt) {
    try {
      const d = JSON.parse(evt.data);
      // Actualizar UI con datos en tiempo real
      if (d.g !== undefined) {
        document.getElementById('hGen').textContent = d.g;
        document.getElementById('sGen').textContent = d.g;
      }
      if (d.p !== undefined) {
        document.getElementById('hPop').textContent = d.p;
        document.getElementById('sPop').textContent = d.p;
        document.getElementById('popCurrent').textContent = d.p;
        // Agregar al historial del gráfico
        popData.push(d.p);
        if (popData.length > MAX_POP_POINTS) popData.shift();
        // Redibujar gráfico si está visible
        const panel = document.getElementById('panelPopgraph');
        if (panel && panel.style.display !== 'none') drawPopGraph();
      }
      if (d.m !== undefined) {
        document.getElementById('sMax').textContent = d.m;
        document.getElementById('popMax').textContent = d.m;
      }
      if (d.r !== undefined) {
        isRunning = d.r === 1;
        document.getElementById('hState').textContent = isRunning ? 'EN EJECUCION' : 'EN PAUSA';
        document.getElementById('hState').style.color = isRunning ? 'var(--accent)' : 'var(--warn)';
        updatePauseBtn();
      }
    } catch(e) {}
  };
}

// ─── Gráfico de Población ───
async function fetchPopHistory() {
  const data = await api('pophistory');
  if (data && Array.isArray(data)) {
    popData = data;
  }
}

function drawPopGraph() {
  const canvas = document.getElementById('popCanvas');
  if (!canvas) return;
  const ctx2 = canvas.getContext('2d');
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * (window.devicePixelRatio || 1);
  canvas.height = rect.height * (window.devicePixelRatio || 1);
  ctx2.scale(window.devicePixelRatio || 1, window.devicePixelRatio || 1);
  const w = rect.width, h = rect.height;
  
  // Fondo
  ctx2.fillStyle = '#000';
  ctx2.fillRect(0, 0, w, h);
  
  if (popData.length < 2) {
    ctx2.fillStyle = '#444';
    ctx2.font = '12px monospace';
    ctx2.textAlign = 'center';
    ctx2.fillText('Esperando datos...', w/2, h/2);
    return;
  }
  
  const maxVal = Math.max(...popData, 1);
  const step = w / (popData.length - 1);
  
  // Cuadrícula
  ctx2.strokeStyle = 'rgba(255,255,255,0.05)';
  ctx2.lineWidth = 0.5;
  for (let i = 0; i < 5; i++) {
    const gy = h * i / 4;
    ctx2.beginPath(); ctx2.moveTo(0, gy); ctx2.lineTo(w, gy); ctx2.stroke();
  }
  
  // Relleno degradado bajo la línea
  const grad = ctx2.createLinearGradient(0, 0, 0, h);
  grad.addColorStop(0, 'rgba(0,255,136,0.3)');
  grad.addColorStop(1, 'rgba(0,255,136,0)');
  ctx2.beginPath();
  ctx2.moveTo(0, h);
  for (let i = 0; i < popData.length; i++) {
    const x = i * step;
    const y = h - (popData[i] / maxVal) * (h - 10);
    ctx2.lineTo(x, y);
  }
  ctx2.lineTo((popData.length - 1) * step, h);
  ctx2.closePath();
  ctx2.fillStyle = grad;
  ctx2.fill();
  
  // Línea principal
  ctx2.beginPath();
  ctx2.strokeStyle = '#00ff88';
  ctx2.lineWidth = 1.5;
  for (let i = 0; i < popData.length; i++) {
    const x = i * step;
    const y = h - (popData[i] / maxVal) * (h - 10);
    if (i === 0) ctx2.moveTo(x, y); else ctx2.lineTo(x, y);
  }
  ctx2.stroke();
  
  // Etiqueta del máximo
  ctx2.fillStyle = '#00ccff';
  ctx2.font = '10px monospace';
  ctx2.textAlign = 'left';
  ctx2.fillText('Max: ' + maxVal, 4, 12);
  ctx2.textAlign = 'right';
  ctx2.fillText(popData[popData.length-1], w - 4, 12);
}

// ─── Patrones Personalizados ───
async function loadCustomPatterns() {
  const data = await api('patterns/list');
  const list = document.getElementById('customPatternList');
  if (!list) return;
  list.innerHTML = '';
  if (!data || data.length === 0) {
    list.innerHTML = '<div style="color:var(--dim);padding:8px">No hay patrones guardados</div>';
    return;
  }
  for (const p of data) {
    const div = document.createElement('div');
    div.style.cssText = 'display:flex;justify-content:space-between;align-items:center;padding:4px 6px;border-bottom:1px solid var(--border)';
    div.innerHTML = '<span style="color:var(--text)">' + p.name + ' <span style="color:var(--dim);font-size:0.8em">(' + p.size + 'B)</span></span>' +
      '<span>' +
      '<button class="btn" style="padding:2px 8px;font-size:0.7em;margin-left:4px" onclick="loadPattern(\'' + p.name + '\')">Cargar</button>' +
      '<button class="btn danger" style="padding:2px 8px;font-size:0.7em;margin-left:4px" onclick="deletePattern(\'' + p.name + '\')">X</button>' +
      '</span>';
    list.appendChild(div);
  }
}

async function savePattern() {
  const name = document.getElementById('patternName').value.trim();
  if (!name) { showToast('Introduce un nombre'); return; }
  const result = await api('patterns/save', 'POST', {name});
  if (result && result.ok) {
    showToast('Patron "' + name + '" guardado');
    document.getElementById('patternName').value = '';
    loadCustomPatterns();
  } else {
    showToast('Error al guardar');
  }
}

async function loadPattern(name) {
  const result = await api('patterns/load', 'POST', {name});
  if (result && result.ok) {
    showToast('Patron "' + name + '" cargado');
    setTimeout(fetchGrid, 200);
  } else {
    showToast('Error al cargar');
  }
}

async function deletePattern(name) {
  if (!confirm('Eliminar patron "' + name + '"?')) return;
  await api('patterns/delete', 'POST', {name});
  showToast('Patron eliminado');
  loadCustomPatterns();
}

// ─── RLE Import/Export ───
async function exportRLE() {
  try {
    const r = await fetch('/api/rle/export');
    const text = await r.text();
    document.getElementById('rleData').value = text;
    showToast('Patron exportado como RLE');
  } catch(e) {
    showToast('Error al exportar');
  }
}

async function importRLE() {
  const rle = document.getElementById('rleData').value.trim();
  if (!rle) { showToast('Pega un patron RLE primero'); return; }
  if (!rle.includes('!')) { showToast('RLE invalido (falta !)'); return; }
  try {
    const r = await fetch('/api/rle/import', {method:'POST',headers:{'Content-Type':'text/plain'},body:rle});
    const data = await r.json();
    if (data.ok) {
      showToast('Patron RLE importado (' + data.pop + ' celulas)');
      setTimeout(fetchGrid, 200);
    } else {
      showToast('Error: ' + (data.error || 'desconocido'));
    }
  } catch(e) {
    showToast('Error al importar');
  }
}

// ─── Inicialización ───
async function init() {
  // Cargar configuración
  const config = await api('config');
  if (config) {
    document.getElementById('speedSlider').value = config.speed;
    updateSpeedLabel();
    currentWebTheme = config.theme;
    
    // Rellenar selector de temas
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
  
  // Cargar cuadrícula
  await fetchGrid();
  resizeCanvas();
  
  // Conectar WebSocket para actualizaciones en tiempo real
  connectWebSocket();
  
  // Polling reducido (WebSocket maneja los datos en tiempo real)
  // El polling HTTP solo se usa como respaldo cada 5 segundos
  pollInterval = setInterval(async () => {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
      // Solo usar HTTP polling si WebSocket no está conectado
      if (isRunning) {
        await fetchGrid();
      } else {
        await fetchStats();
      }
    }
  }, 5000);
}

window.addEventListener('resize', resizeCanvas);
init();
</script>
</body>
</html>
)rawliteral";
