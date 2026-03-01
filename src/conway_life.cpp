/*
 * ══════════════════════════════════════════════════════════════
 *  EL JUEGO DE LA VIDA DE CONWAY - ESP8266 + ILI9341 Pantalla Táctil
 * ══════════════════════════════════════════════════════════════
 *  
 *  Autor:    wencescarlos
 *  GitHub:   https://github.com/wencescarlos/conways_life
 *  Versión:  1.0
 *  Licencia: MIT
 *  
 *  Hardware: Wemos D1 Mini (ESP8266) + ILI9341 2.8" TFT Táctil
 *  
 *  Características:
 *   - Simulación completa del Juego de la Vida de Conway en pantalla TFT
 *   - Interacción por pantalla táctil para dibujar células
 *   - Interfaz web con WebSocket en tiempo real
 *   - Actualización de firmware OTA vía web (protegida con contraseña)
 *   - Portal cautivo en modo AP
 *   - Guardar/cargar patrones personalizados en SPIFFS
 *   - Exportar/importar patrones en formato RLE
 *   - Gráfico de población en tiempo real en la web
 *   - 20 temas de color seleccionables
 *   - Biblioteca de patrones (Planeador, Pulsar, Cañón de Gosper, etc.)
 *   - Seguimiento de estadísticas (generaciones, reinicios, población máxima)
 *  
 *  Conexiones de pines (Wemos D1 Mini -> ILI9341):
 *   D1 (GPIO5)  -> CS          (Selección de chip TFT)
 *   D2 (GPIO4)  -> DC/RS       (Datos/Comando)
 *   D8 (GPIO15) -> LED         (Retroiluminación, controlable por PWM)
 *   D3 (GPIO0)  -> T_CS        (Selección de chip táctil)
 *   D4 (GPIO2)  -> T_IRQ       (Interrupción táctil)
 *   D7 (GPIO13) -> MOSI/SDA    (SPI compartido)
 *   D5 (GPIO14) -> SCK/CLK     (SPI compartido)
 *   D6 (GPIO12) -> MISO/SDO    (SPI compartido)
 *   3.3V        -> VCC
 *   GND         -> GND
 *   RST         -> RST (o 3.3V mediante resistencia)
 *  
 * ══════════════════════════════════════════════════════════════
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>  // Actualización OTA vía web
#include <ESP8266mDNS.h>
#include <DNSServer.h>                // Portal cautivo
#include <WebSocketsServer.h>         // WebSocket tiempo real
#include <EEPROM.h>
#include <FS.h>                       // SPIFFS para patrones personalizados
#include <SPI.h>
#include <TFT_eSPI.h>                 // Biblioteca TFT de Bodmer
#include <ArduinoJson.h>
#include <time.h>

// ─── Información del Firmware ──────────────────────────────────
#define FW_VERSION    "1.0"
#define FW_AUTHOR     "wencescarlos"
#define FW_GITHUB     "https://github.com/wencescarlos/conways_life"
#define FW_NAME       "Conway's Life ESP8266"

// ─── Credenciales OTA ──────────────────────────────────────────
#define OTA_USER      "admin"
#define OTA_PASS      "conway1234"

// ─── Portal Cautivo ────────────────────────────────────────────
#define DNS_PORT      53

// ─── Configuración de Pantalla y Cuadrícula ────────────────────
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define CELL_SIZE     4          // Cada célula = 4x4 píxeles
#define GRID_COLS     (SCREEN_WIDTH / CELL_SIZE)   // 80 columnas
#define GRID_ROWS     ((SCREEN_HEIGHT - 20) / CELL_SIZE)  // 55 filas (20px para barra de estado)
#define GRID_BYTES    ((GRID_COLS * GRID_ROWS + 7) / 8)
#define STATUS_BAR_Y  (GRID_ROWS * CELL_SIZE)

// ─── Definiciones de Pines (ajustar según tu cableado) ─────────
//  TFT_CS, TFT_DC, TOUCH_CS se configuran en User_Setup.h para TFT_eSPI
//  Estos pines se usan directamente en el código para retroiluminación e IRQ táctil:
#define TFT_LED       D8    // GPIO15 - Retroiluminación (PWM)
#define TOUCH_IRQ_PIN D4    // GPIO2  - Interrupción táctil

// ─── Calibración Táctil ───────────────────────────────────────
#define TOUCH_MIN_X   200
#define TOUCH_MAX_X   3700
#define TOUCH_MIN_Y   300
#define TOUCH_MAX_Y   3800
#define TOUCH_THRESHOLD 600

// ─── Distribución de EEPROM ───────────────────────────────────
#define EEPROM_SIZE       512
#define EEPROM_MAGIC      0xC0  // Byte mágico para detectar configuración válida
#define EEPROM_ADDR_MAGIC 0
#define EEPROM_ADDR_SSID  1     // 32 bytes
#define EEPROM_ADDR_PASS  33    // 64 bytes
#define EEPROM_ADDR_THEME 97    // 1 byte
#define EEPROM_ADDR_SPEED 98    // 2 bytes (retardo en ms)
#define EEPROM_ADDR_SCHON 100   // 1 byte (hora de encendido programado)
#define EEPROM_ADDR_SCHOFF 101  // 1 byte (hora de apagado programado)
#define EEPROM_ADDR_SCHEN 102   // 1 byte (programación activada)
#define EEPROM_ADDR_BRIGHT 103  // 1 byte (brillo 0-100)
#define EEPROM_ADDR_GAMES 104   // 4 bytes (contador total de partidas)
#define EEPROM_ADDR_GENS  108   // 4 bytes (generaciones totales)
#define EEPROM_ADDR_NAME  112   // 32 bytes (nombre del dispositivo)
#define EEPROM_ADDR_TZ    144   // 1 byte (zona horaria, int8_t: -12 a +14)

// ─── Temas de Color ───────────────────────────────────────────
struct TemaColor {
  const char* nombre;
  uint16_t viva;
  uint16_t muerta;
  uint16_t fondoEstado;
  uint16_t textoEstado;
  uint16_t cuadricula;
  uint16_t muriendo;     // Color para células a punto de morir (efecto visual)
  uint16_t recienNacida; // Color para células recién nacidas
};

//  Formato RGB565: (R5 << 11) | (G6 << 5) | B5
//  Cada tema: {nombre, viva, muerta, fondoEstado, textoEstado, cuadrícula, muriendo, recienNacida}
const TemaColor themes[] = {
  // ── Clásicos ──
  {"Matrix",      0x07E0, 0x0000, 0x0200, 0x07E0, 0x0100, 0x0380, 0x0FE0},  // Verde fósforo sobre negro
  {"Monocromo",   0xFFFF, 0x0000, 0x4208, 0xFFFF, 0x2104, 0x8410, 0xFFFF},  // Blanco puro sobre negro
  {"Ambar",       0xFD00, 0x1000, 0x2100, 0xFD00, 0x1080, 0xFC00, 0xFFE0},  // Terminal ámbar retro
  {"Fosforo",     0x47E0, 0x0000, 0x0200, 0x47E0, 0x0100, 0x2380, 0x67E0},  // Verde P1 fósforo CRT
  
  // ── Naturaleza ──
  {"Oceano",      0x07FF, 0x0008, 0x0010, 0x07FF, 0x0004, 0x03BF, 0x5FFF},  // Cian sobre azul profundo
  {"Bosque",      0x2EC4, 0x0200, 0x0300, 0x4FE6, 0x0180, 0x1682, 0x87E8},  // Verdes de bosque
  {"Atardecer",   0xFD20, 0x2000, 0x4000, 0xFFE0, 0x1000, 0xF800, 0xFFE0},  // Naranja-rojo al anochecer
  {"Aurora",      0x07F4, 0x0008, 0x000C, 0x3FFC, 0x0004, 0x03DA, 0xAFFE},  // Aurora boreal
  {"Coral",       0xFBCD, 0x1082, 0x2104, 0xFE76, 0x0841, 0xF2A8, 0xFED7},  // Arrecife de coral
  
  // ── Energía ──
  {"Lava",        0xF800, 0x1000, 0x2000, 0xFBE0, 0x0800, 0xFC00, 0xFFE0},  // Rojo magma ardiente
  {"Neon",        0xF81F, 0x0000, 0x1002, 0xF81F, 0x0801, 0x780F, 0xFFE0},  // Magenta neón vibrante
  {"Electrico",   0x041F, 0x0000, 0x0008, 0x841F, 0x0004, 0x020F, 0xC61F},  // Azul eléctrico
  {"Plasma",      0xE81C, 0x0801, 0x1002, 0xF83F, 0x0801, 0xA00E, 0xFE1F},  // Plasma violeta-rosa
  {"Solar",       0xFFE0, 0x2100, 0x3180, 0xFFE0, 0x1080, 0xFCC0, 0xFFF8},  // Amarillo solar brillante
  
  // ── Ambientes ──
  {"Hielo",       0xBFFF, 0x000A, 0x0015, 0xFFFF, 0x0005, 0x5BBF, 0xFFFF},  // Cristales de hielo
  {"Medianoche",  0x3A1F, 0x0000, 0x0802, 0x7C1F, 0x0401, 0x1C0F, 0xBC3F},  // Púrpura de medianoche
  {"Desierto",    0xE587, 0x2945, 0x39C7, 0xEE8C, 0x18C3, 0xCC45, 0xF70F},  // Arenas del desierto
  {"Crepusculo",  0xC819, 0x0801, 0x1002, 0xF89F, 0x0801, 0x9013, 0xF89F}, // Rosa crepúsculo
  
  // ── Especiales ──
  {"Retro",       0xFFE0, 0x18C3, 0x2945, 0xFFE0, 0x2104, 0xFCC0, 0x07FF},  // Estilo 8-bit retro
  {"Cianotipia",  0x0410, 0x0008, 0x0010, 0x0618, 0x0004, 0x0210, 0x0E3C},  // Estilo cianotipo fotográfico
};
#define NUM_THEMES (sizeof(themes) / sizeof(themes[0]))

// ─── Estados de la Aplicación ─────────────────────────────────
enum EstadoApp {
  STATE_RUNNING,       // El juego está en ejecución
  STATE_PAUSED,        // El juego está pausado (edición táctil)
  STATE_MENU,          // Menú principal en pantalla
  STATE_WIFI_CONFIG,   // Pantalla de configuración WiFi
  STATE_PATTERN_SELECT,// Pantalla de selección de patrones
  STATE_CONFIG,        // Pantalla de configuración general
  STATE_THEME_SELECT   // Pantalla de selección de tema
};

// ─── Objetos Globales ──────────────────────────────────────────
TFT_eSPI tft = TFT_eSPI();
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer servidorOTA;  // Servidor de actualización OTA
DNSServer dnsServer;                  // DNS para portal cautivo
WebSocketsServer webSocket(81);      // WebSocket en puerto 81

// ─── Historial de Población (para gráfico web) ────────────────
#define POP_HISTORY_SIZE 200
uint16_t popHistory[POP_HISTORY_SIZE];
uint8_t popHistoryIdx = 0;
bool popHistoryFull = false;

// ─── Patrones Personalizados (SPIFFS) ─────────────────────────
#define MAX_CUSTOM_PATTERNS 10
#define PATTERN_DIR "/patterns"

// ─── Búferes de Cuadrícula (empaquetados en bits para eficiencia de memoria) ─
uint8_t gridA[GRID_BYTES];     // Generación actual
uint8_t gridB[GRID_BYTES];     // Siguiente generación
uint8_t* currentGrid = gridA;
uint8_t* nextGrid = gridB;
uint8_t prevGrid[GRID_BYTES];  // Para detectar cambios (optimizar el dibujado)

// ─── Variables de Estado ───────────────────────────────────────
EstadoApp appState = STATE_RUNNING;
bool wifiConectado = false;
bool modoAP = false;
bool pantallaEncendida = true;
bool programacionActivada = false;
uint8_t horaEncendido = 7;
uint8_t horaApagado = 23;
uint8_t temaActual = 0;
uint16_t velocidadSim = 150;      // ms entre generaciones
uint8_t brillo = 100;
uint32_t partidasTotales = 0;
uint32_t generacionesTotales = 0;
uint32_t generacionActual = 0;
uint32_t poblacionMaxima = 0;
uint16_t poblacionActual = 0;
uint16_t contadorEstancamiento = 0;  // Detectar osciladores/estancamiento
uint16_t poblacionAnterior = 0;
uint32_t ultimoTiempoSim = 0;
uint32_t ultimoTiempoTactil = 0;
uint32_t ultimaActualizacionEstado = 0;
String nombreDispositivo = "ConwayLife";
String wifiSSID = "";
String wifiPass = "";
int8_t zonaHoraria = 1;           // Desplazamiento UTC en horas (por defecto CET +1)

// ─── Variables táctiles ────────────────────────────────────────
bool modoTactilDibujar = true;   // true = dibujar vivas, false = borrar
int seleccionMenu = 0;
int cantidadElementosMenu = 0;
uint8_t paginaTemas = 0;           // Página actual del selector de temas

// ─── Bandera de actualización web ──────────────────────────────
volatile bool cuadriculaWebActualizada = false;

// ─── Declaraciones Anticipadas ─────────────────────────────────
void configurarWiFi();
void iniciarModoAP();
void configurarServidorWeb();
void configurarPantalla();
void establecerRetroiluminacion(uint8_t nivel);
void cargarConfiguracion();
void guardarConfiguracion();
void guardarEstadisticas();
void limpiarCuadricula(uint8_t* grid);
void aleatorizarCuadricula();
void calcularSiguienteGeneracion();
void dibujarCuadricula(bool redibujadoCompleto);
void dibujarBarraEstado();
void dibujarPantallaInicio();
void dibujarCelula(int col, int fila, bool viva, bool esNueva = false, bool estaMuriendo = false);
void manejarTactil();
void dibujarMenuPrincipal();
void dibujarConfigWiFi();
void dibujarSeleccionPatron();
void manejarTactilMenu(int x, int y);
void manejarTactilConfigWiFi(int x, int y);
void manejarTactilPatron(int x, int y);
void dibujarConfigGeneral();
void manejarTactilConfig(int x, int y);
void aplicarZonaHoraria();
void dibujarSeleccionTema();
void manejarTactilTema(int x, int y);
void dibujarBotonConfig(int x, int y, int w, int h, const char* texto, uint16_t colorFondo, uint16_t colorTexto);
void insertarPatron(int tipo, int cx, int cy);
void colocarPatron(const int8_t pat[][2], int cantidad, int ox, int oy);
void verificarProgramacion();
void verificarEstancamiento();
void manejarEventoWebSocket(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
inline bool obtenerCelula(uint8_t* grid, int col, int fila);
inline void establecerCelula(uint8_t* grid, int col, int fila, bool val);
int contarVecinos(uint8_t* grid, int col, int fila);
uint16_t contarPoblacion();

// Declaraciones anticipadas de manejadores web
void manejarRaiz();
void manejarObtenerCuadricula();
void manejarEstablecerCuadricula();
void manejarEstablecerCelula();
void manejarLimpiar();
void manejarAleatorio();
void manejarPausar();
void manejarReanudar();
void manejarPaso();
void manejarVelocidad();
void manejarTema();
void manejarPatron();
void manejarEstadisticas();
void manejarConfigWiFi();
void manejarEscaneoWiFi();
void manejarProgramacion();
void manejarObtenerProgramacion();
void manejarAlternarPantalla();
void manejarBrillo();
void manejarObtenerConfig();

// Contenido de página web almacenado en PROGMEM
#include "web_page.h"

// ═════════════════════════════════════════════════════════════
//  CONFIGURACIÓN INICIAL
// ═════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  Serial.println(F("\n\n═══════════════════════════════════════════"));
  Serial.println(F("  El Juego de la Vida de Conway"));
  Serial.printf( "  Firmware v%s por %s\n", FW_VERSION, FW_AUTHOR);
  Serial.printf( "  %s\n", FW_GITHUB);
  Serial.println(F("═══════════════════════════════════════════"));
  
  // Inicializar EEPROM
  EEPROM.begin(EEPROM_SIZE);
  cargarConfiguracion();
  
  // Inicializar SPIFFS para patrones personalizados
  if (SPIFFS.begin()) {
    Serial.println(F("SPIFFS inicializado"));
    // Crear directorio de patrones si no existe
    if (!SPIFFS.exists(PATTERN_DIR)) {
      // SPIFFS no usa directorios reales, los archivos usan la ruta completa
      Serial.println(F("Directorio de patrones listo"));
    }
  } else {
    Serial.println(F("Error al inicializar SPIFFS"));
  }
  
  // Inicializar historial de población
  memset(popHistory, 0, sizeof(popHistory));
  
  // Inicializar Pantalla
  configurarPantalla();
  
  // Mostrar pantalla de inicio
  dibujarPantallaInicio();
  delay(2000);
  
  // Configurar WiFi
  configurarWiFi();
  
  // Configurar Servidor Web
  configurarServidorWeb();
  
  // Configurar mDNS
  if (wifiConectado) {
    MDNS.begin(nombreDispositivo.c_str());
    MDNS.addService("http", "tcp", 80);
    Serial.printf("mDNS: http://%s.local\n", nombreDispositivo.c_str());
  }
  
  // Configurar portal cautivo en modo AP
  if (modoAP) {
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    Serial.println(F("Portal cautivo DNS iniciado"));
  }
  
  // Configurar WebSocket
  webSocket.begin();
  webSocket.onEvent(manejarEventoWebSocket);
  Serial.println(F("WebSocket iniciado en puerto 81"));
  
  // Configurar NTP para programación horaria
  if (wifiConectado) {
    aplicarZonaHoraria();
    Serial.println(F("NTP configurado"));
  }
  
  // Inicializar cuadrícula con patrón aleatorio
  aleatorizarCuadricula();
  
  // Dibujado completo inicial
  tft.fillScreen(themes[temaActual].muerta);
  dibujarCuadricula(true);
  dibujarBarraEstado();
  
  Serial.printf("Cuadrícula: %dx%d células, Tamaño de célula: %dpx\n", GRID_COLS, GRID_ROWS, CELL_SIZE);
  Serial.printf("Heap libre: %d bytes\n", ESP.getFreeHeap());
}

// ═════════════════════════════════════════════════════════════
//  BUCLE PRINCIPAL
// ═════════════════════════════════════════════════════════════
void loop() {
  // Manejar servidor web y WebSocket
  server.handleClient();
  webSocket.loop();
  
  // Manejar DNS del portal cautivo
  if (modoAP) {
    dnsServer.processNextRequest();
  }
  
  if (wifiConectado) {
    MDNS.update();
  }
  
  // Manejar entrada táctil
  manejarTactil();
  
  // Verificar programación de pantalla
  verificarProgramacion();
  
  // Actualización de cuadrícula desde la web
  if (cuadriculaWebActualizada) {
    cuadriculaWebActualizada = false;
    if (pantallaEncendida) {
      // Limpiar toda el área de la cuadrícula antes de redibujar
      tft.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_Y, themes[temaActual].muerta);
      dibujarCuadricula(true);
      dibujarBarraEstado();
    }
  }
  
  // Ejecutar simulación
  if (appState == STATE_RUNNING && millis() - ultimoTiempoSim >= velocidadSim) {
    ultimoTiempoSim = millis();
    
    // Guardar cuadrícula actual para dibujado diferencial
    memcpy(prevGrid, currentGrid, GRID_BYTES);
    
    // Calcular siguiente generación
    calcularSiguienteGeneracion();
    
    // Intercambiar búferes
    uint8_t* temp = currentGrid;
    currentGrid = nextGrid;
    nextGrid = temp;
    
    generacionActual++;
    generacionesTotales++;
    poblacionActual = contarPoblacion();
    
    if (poblacionActual > poblacionMaxima) {
      poblacionMaxima = poblacionActual;
    }
    
    // Dibujar cambios
    if (pantallaEncendida) {
      dibujarCuadricula(false);
    }
    
    // Actualizar barra de estado periódicamente
    if (millis() - ultimaActualizacionEstado > 500) {
      ultimaActualizacionEstado = millis();
      if (pantallaEncendida) dibujarBarraEstado();
      
      // Registrar población en historial para gráfico web
      popHistory[popHistoryIdx] = poblacionActual;
      popHistoryIdx = (popHistoryIdx + 1) % POP_HISTORY_SIZE;
      if (popHistoryIdx == 0) popHistoryFull = true;
      
      // Enviar datos por WebSocket a todos los clientes conectados
      if (webSocket.connectedClients() > 0) {
        String wsMsg = "{\"g\":" + String(generacionActual) +
                       ",\"p\":" + String(poblacionActual) +
                       ",\"m\":" + String(poblacionMaxima) +
                       ",\"r\":" + String(appState == STATE_RUNNING ? 1 : 0) + "}";
        webSocket.broadcastTXT(wsMsg);
      }
    }
    
    // Verificar extinción o estancamiento
    verificarEstancamiento();
    
    // Guardar estadísticas cada 100 generaciones
    if (generacionActual % 100 == 0) {
      guardarEstadisticas();
    }
    
    yield(); // Alimentar el watchdog
  }
}

// ═════════════════════════════════════════════════════════════
//  CONFIGURACIÓN DE PANTALLA
// ═════════════════════════════════════════════════════════════
void configurarPantalla() {
  // Inicializar pin de retroiluminación con PWM
  pinMode(TFT_LED, OUTPUT);
  analogWriteRange(1023);        // Resolución PWM de 10 bits
  analogWriteFreq(1000);         // Frecuencia PWM de 1kHz
  establecerRetroiluminacion(brillo);  // Establecer brillo guardado
  
  // Pin IRQ táctil (activo en BAJO al tocar)
  pinMode(TOUCH_IRQ_PIN, INPUT_PULLUP);
  
  tft.init();
  tft.setRotation(1);  // Apaisado (horizontal)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  Serial.println(F("Pantalla inicializada"));
}

// ═════════════════════════════════════════════════════════════
//  CONTROL DE RETROILUMINACIÓN (PWM)
// ═════════════════════════════════════════════════════════════
void establecerRetroiluminacion(uint8_t nivel) {
  // nivel: 0-100 (porcentaje)
  nivel = constrain(nivel, 0, 100);
  uint16_t pwm = map(nivel, 0, 100, 0, 1023);
  analogWrite(TFT_LED, pwm);
}

// ═════════════════════════════════════════════════════════════
//  PANTALLA DE INICIO
// ═════════════════════════════════════════════════════════════
void dibujarPantallaInicio() {
  tft.fillScreen(TFT_BLACK);
  
  // Dibujar células de Conway animadas como decoración
  for (int i = 0; i < 40; i++) {
    int x = random(0, 320);
    int y = random(0, 240);
    int s = random(3, 8);
    tft.fillRect(x, y, s, s, themes[temaActual].viva);
  }
  
  // Título
  tft.setTextSize(3);
  tft.setTextColor(themes[temaActual].viva);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("EL JUEGO DE", 160, 55);
  tft.drawString("LA VIDA", 160, 90);
  
  // Versión
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  char verBuf[40];
  sprintf(verBuf, "Firmware v%s por %s", FW_VERSION, FW_AUTHOR);
  tft.drawString(verBuf, 160, 118);
  
  // GitHub
  tft.setTextColor(0x04BF); // Azul claro
  tft.drawString("github.com/wencescarlos/conways_life", 160, 132);
  
  // Estadísticas
  tft.setTextColor(themes[temaActual].textoEstado);
  tft.setTextSize(1);
  char buf[50];
  sprintf(buf, "Partidas jugadas: %lu", partidasTotales);
  tft.drawString(buf, 160, 155);
  sprintf(buf, "Generaciones totales: %lu", generacionesTotales);
  tft.drawString(buf, 160, 170);
  
  tft.drawString("Toca la pantalla para comenzar...", 160, 210);
  tft.setTextDatum(TL_DATUM);
}

// ═════════════════════════════════════════════════════════════
//  CONFIGURACIÓN WIFI
// ═════════════════════════════════════════════════════════════
void configurarWiFi() {
  if (wifiSSID.length() > 0) {
    // Intentar conectar al WiFi guardado
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 50);
    tft.println("Conectando WiFi...");
    tft.setTextSize(1);
    tft.setCursor(10, 80);
    tft.print("SSID: ");
    tft.println(wifiSSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
    
    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 30) {
      delay(500);
      tft.print(".");
      intentos++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiConectado = true;
      modoAP = false;
      tft.println();
      tft.setTextSize(2);
      tft.setTextColor(TFT_GREEN);
      tft.setCursor(10, 110);
      tft.println("Conectado!");
      tft.setTextSize(1);
      tft.setCursor(10, 140);
      tft.print("IP: ");
      tft.println(WiFi.localIP().toString());
      Serial.printf("WiFi conectado! IP: %s\n", WiFi.localIP().toString().c_str());
      delay(1500);
      return;
    }
  }
  
  // Iniciar modo AP
  iniciarModoAP();
}

void iniciarModoAP() {
  modoAP = true;
  wifiConectado = false;
  
  String apSSID = "ConwayLife-" + String(ESP.getChipId(), HEX);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID.c_str(), "conway1234");
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 30);
  tft.println("Modo AP Activo");
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 60);
  tft.print("Red: ");
  tft.println(apSSID);
  tft.setCursor(10, 75);
  tft.println("Clave: conway1234");
  tft.setCursor(10, 95);
  tft.print("Config: http://");
  tft.println(WiFi.softAPIP().toString());
  tft.setCursor(10, 120);
  tft.setTextColor(TFT_DARKGREY);
  tft.println("Toca para continuar...");
  
  Serial.printf("Modo AP: %s / conway1234\n", apSSID.c_str());
  Serial.printf("URL Config: http://%s\n", WiFi.softAPIP().toString().c_str());
  
  delay(3000);
}

// ═════════════════════════════════════════════════════════════
//  CONFIGURACIÓN DEL SERVIDOR WEB
// ═════════════════════════════════════════════════════════════
void configurarServidorWeb() {
  // Página principal
  server.on("/", HTTP_GET, manejarRaiz);
  
  // Puntos de acceso API
  server.on("/api/grid", HTTP_GET, manejarObtenerCuadricula);
  server.on("/api/grid", HTTP_POST, manejarEstablecerCuadricula);
  server.on("/api/cell", HTTP_POST, manejarEstablecerCelula);
  server.on("/api/clear", HTTP_POST, manejarLimpiar);
  server.on("/api/random", HTTP_POST, manejarAleatorio);
  server.on("/api/pause", HTTP_POST, manejarPausar);
  server.on("/api/resume", HTTP_POST, manejarReanudar);
  server.on("/api/step", HTTP_POST, manejarPaso);
  server.on("/api/speed", HTTP_POST, manejarVelocidad);
  server.on("/api/theme", HTTP_POST, manejarTema);
  server.on("/api/pattern", HTTP_POST, manejarPatron);
  server.on("/api/stats", HTTP_GET, manejarEstadisticas);
  server.on("/api/wifi", HTTP_POST, manejarConfigWiFi);
  server.on("/api/wifi/scan", HTTP_GET, manejarEscaneoWiFi);
  server.on("/api/schedule", HTTP_POST, manejarProgramacion);
  server.on("/api/schedule", HTTP_GET, manejarObtenerProgramacion);
  server.on("/api/screen", HTTP_POST, manejarAlternarPantalla);
  server.on("/api/brightness", HTTP_POST, manejarBrillo);
  server.on("/api/config", HTTP_GET, manejarObtenerConfig);
  
  // Configurar servidor de actualización OTA (protegido con contraseña)
  // Accesible en http://IP/update (formulario de subida de firmware)
  servidorOTA.setup(&server, "/update", OTA_USER, OTA_PASS);
  
  // Endpoint de información de firmware
  server.on("/api/firmware", HTTP_GET, []() {
    String json = "{";
    json += "\"version\":\"" + String(FW_VERSION) + "\"";
    json += ",\"author\":\"" + String(FW_AUTHOR) + "\"";
    json += ",\"github\":\"" + String(FW_GITHUB) + "\"";
    json += ",\"name\":\"" + String(FW_NAME) + "\"";
    json += ",\"chipId\":\"" + String(ESP.getChipId(), HEX) + "\"";
    json += ",\"flashSize\":" + String(ESP.getFlashChipRealSize());
    json += ",\"sketchSize\":" + String(ESP.getSketchSize());
    json += ",\"freeSketch\":" + String(ESP.getFreeSketchSpace());
    json += ",\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\"";
    json += ",\"otaUser\":\"" + String(OTA_USER) + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });
  
  // ── Endpoint: Historial de población (para gráfico web) ──
  server.on("/api/pophistory", HTTP_GET, []() {
    String json = "[";
    int count = popHistoryFull ? POP_HISTORY_SIZE : popHistoryIdx;
    int start = popHistoryFull ? popHistoryIdx : 0;
    for (int i = 0; i < count; i++) {
      if (i > 0) json += ",";
      json += String(popHistory[(start + i) % POP_HISTORY_SIZE]);
    }
    json += "]";
    server.send(200, "application/json", json);
  });
  
  // ── Endpoints: Patrones personalizados (SPIFFS) ──
  server.on("/api/patterns/list", HTTP_GET, []() {
    String json = "[";
    Dir dir = SPIFFS.openDir(PATTERN_DIR);
    bool first = true;
    while (dir.next()) {
      if (!first) json += ",";
      String name = dir.fileName();
      name.replace(String(PATTERN_DIR) + "/", "");
      name.replace(".json", "");
      json += "{\"name\":\"" + name + "\",\"size\":" + String(dir.fileSize()) + "}";
      first = false;
    }
    json += "]";
    server.send(200, "application/json", json);
  });
  
  server.on("/api/patterns/save", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"sin cuerpo\"}");
      return;
    }
    StaticJsonDocument<256> doc;
    deserializeJson(doc, server.arg("plain"));
    String name = doc["name"].as<String>();
    if (name.length() == 0 || name.length() > 20) {
      server.send(400, "application/json", "{\"error\":\"nombre invalido\"}");
      return;
    }
    // Guardar cuadrícula actual como patrón
    String path = String(PATTERN_DIR) + "/" + name + ".json";
    File f = SPIFFS.open(path, "w");
    if (!f) {
      server.send(500, "application/json", "{\"error\":\"error SPIFFS\"}");
      return;
    }
    f.print("{\"cols\":" + String(GRID_COLS) + ",\"rows\":" + String(GRID_ROWS) + ",\"cells\":[");
    bool first = true;
    for (int r = 0; r < GRID_ROWS; r++) {
      for (int c = 0; c < GRID_COLS; c++) {
        if (obtenerCelula(currentGrid, c, r)) {
          if (!first) f.print(",");
          f.print("[" + String(c) + "," + String(r) + "]");
          first = false;
        }
      }
    }
    f.print("]}");
    f.close();
    server.send(200, "application/json", "{\"ok\":true}");
  });
  
  server.on("/api/patterns/load", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"sin cuerpo\"}");
      return;
    }
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    String name = doc["name"].as<String>();
    String path = String(PATTERN_DIR) + "/" + name + ".json";
    if (!SPIFFS.exists(path)) {
      server.send(404, "application/json", "{\"error\":\"no encontrado\"}");
      return;
    }
    File f = SPIFFS.open(path, "r");
    String content = f.readString();
    f.close();
    // Parsear y aplicar
    DynamicJsonDocument pdoc(8192);
    deserializeJson(pdoc, content);
    limpiarCuadricula(currentGrid);
    JsonArray cells = pdoc["cells"].as<JsonArray>();
    for (JsonArray cell : cells) {
      int c = cell[0]; int r = cell[1];
      if (c >= 0 && c < GRID_COLS && r >= 0 && r < GRID_ROWS) {
        establecerCelula(currentGrid, c, r, true);
      }
    }
    generacionActual = 0;
    poblacionActual = contarPoblacion();
    appState = STATE_PAUSED;
    cuadriculaWebActualizada = true;
    server.send(200, "application/json", "{\"ok\":true}");
  });
  
  server.on("/api/patterns/delete", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"sin cuerpo\"}");
      return;
    }
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    String name = doc["name"].as<String>();
    String path = String(PATTERN_DIR) + "/" + name + ".json";
    if (SPIFFS.exists(path)) {
      SPIFFS.remove(path);
    }
    server.send(200, "application/json", "{\"ok\":true}");
  });
  
  // ── Endpoint: Exportar patrón actual como RLE ──
  server.on("/api/rle/export", HTTP_GET, []() {
    // Encontrar bounding box de células vivas
    int minC = GRID_COLS, maxC = 0, minR = GRID_ROWS, maxR = 0;
    for (int r = 0; r < GRID_ROWS; r++) {
      for (int c = 0; c < GRID_COLS; c++) {
        if (obtenerCelula(currentGrid, c, r)) {
          if (c < minC) minC = c; if (c > maxC) maxC = c;
          if (r < minR) minR = r; if (r > maxR) maxR = r;
        }
      }
    }
    if (minC > maxC) { // Cuadrícula vacía
      server.send(200, "text/plain", "#C Empty grid\nx = 0, y = 0, rule = B3/S23\n!\n");
      return;
    }
    int w = maxC - minC + 1;
    int h = maxR - minR + 1;
    String rle = "#C Conway's Life - " + String(FW_GITHUB) + "\n";
    rle += "#C Exported from " + String(FW_NAME) + " v" + String(FW_VERSION) + "\n";
    rle += "x = " + String(w) + ", y = " + String(h) + ", rule = B3/S23\n";
    // Codificar RLE
    for (int r = minR; r <= maxR; r++) {
      int runCount = 0;
      bool runState = false;
      for (int c = minC; c <= maxC; c++) {
        bool alive = obtenerCelula(currentGrid, c, r);
        if (c == minC) { runState = alive; runCount = 1; continue; }
        if (alive == runState) { runCount++; }
        else {
          if (runCount > 1) rle += String(runCount);
          rle += runState ? "o" : "b";
          runState = alive; runCount = 1;
        }
      }
      // Escribir último run de la fila
      if (runState) { // Solo escribir si son células vivas (b's al final se omiten)
        if (runCount > 1) rle += String(runCount);
        rle += "o";
      }
      rle += (r < maxR) ? "$" : "!";
    }
    rle += "\n";
    server.send(200, "text/plain", rle);
  });
  
  // ── Endpoint: Importar patrón RLE ──
  server.on("/api/rle/import", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"sin cuerpo\"}");
      return;
    }
    String rleData = server.arg("plain");
    limpiarCuadricula(currentGrid);
    // Parsear RLE simple
    int x = 0, y = 0;
    int ox = GRID_COLS / 4; // Offset para centrar
    int oy = GRID_ROWS / 4;
    int runCount = 0;
    bool inHeader = true;
    for (unsigned int i = 0; i < rleData.length(); i++) {
      char ch = rleData[i];
      if (ch == '#') { while (i < rleData.length() && rleData[i] != '\n') i++; continue; }
      if (inHeader && ch == '\n') { inHeader = false; continue; }
      if (inHeader) {
        if (ch == 'x') inHeader = false; // Después de la línea x=...
        continue;
      }
      if (ch >= '0' && ch <= '9') { runCount = runCount * 10 + (ch - '0'); continue; }
      if (runCount == 0) runCount = 1;
      if (ch == 'b') { x += runCount; }
      else if (ch == 'o') {
        for (int j = 0; j < runCount; j++) {
          if (ox + x < GRID_COLS && oy + y < GRID_ROWS)
            establecerCelula(currentGrid, ox + x, oy + y, true);
          x++;
        }
      }
      else if (ch == '$') { y += runCount; x = 0; }
      else if (ch == '!') break;
      runCount = 0;
    }
    generacionActual = 0;
    poblacionActual = contarPoblacion();
    appState = STATE_PAUSED;
    cuadriculaWebActualizada = true;
    server.send(200, "application/json", "{\"ok\":true,\"pop\":" + String(poblacionActual) + "}");
  });
  
  // ── Portal cautivo: redirigir todas las rutas desconocidas ──
  server.onNotFound([]() {
    if (modoAP) {
      server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "No encontrado");
    }
  });
  
  server.begin();
  Serial.println(F("Servidor web iniciado (OTA en /update, WS en :81)"));
}

// ═════════════════════════════════════════════════════════════
//  MANEJADORES WEB
// ═════════════════════════════════════════════════════════════
void manejarRaiz() {
  server.send_P(200, "text/html", WEB_PAGE);
}

void manejarObtenerCuadricula() {
  String json = "{\"cols\":" + String(GRID_COLS) + ",\"rows\":" + String(GRID_ROWS) + ",\"cells\":[";
  bool primero = true;
  for (int f = 0; f < GRID_ROWS; f++) {
    for (int c = 0; c < GRID_COLS; c++) {
      if (obtenerCelula(currentGrid, c, f)) {
        if (!primero) json += ",";
        json += "[" + String(c) + "," + String(f) + "]";
        primero = false;
      }
    }
  }
  json += "],\"gen\":" + String(generacionActual);
  json += ",\"pop\":" + String(poblacionActual);
  json += ",\"games\":" + String(partidasTotales);
  json += ",\"running\":" + String(appState == STATE_RUNNING ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

void manejarEstablecerCuadricula() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"sin cuerpo\"}");
    return;
  }
  
  StaticJsonDocument<8192> doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "application/json", "{\"error\":\"json invalido\"}");
    return;
  }
  
  limpiarCuadricula(currentGrid);
  JsonArray cells = doc["cells"].as<JsonArray>();
  for (JsonArray cell : cells) {
    int c = cell[0];
    int f = cell[1];
    if (c >= 0 && c < GRID_COLS && f >= 0 && f < GRID_ROWS) {
      establecerCelula(currentGrid, c, f, true);
    }
  }
  
  generacionActual = 0;
  poblacionActual = contarPoblacion();
  // Pausar simulación al cargar cuadrícula desde la web
  appState = STATE_PAUSED;
  cuadriculaWebActualizada = true;
  server.send(200, "application/json", "{\"ok\":true}");
}

void manejarEstablecerCelula() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"sin cuerpo\"}");
    return;
  }
  
  StaticJsonDocument<256> doc;
  deserializeJson(doc, server.arg("plain"));
  int c = doc["col"];
  int f = doc["row"];
  bool val = doc["alive"] | true;
  
  if (c >= 0 && c < GRID_COLS && f >= 0 && f < GRID_ROWS) {
    establecerCelula(currentGrid, c, f, val);
    poblacionActual = contarPoblacion();
    // Pausar simulación al editar desde la web (modo edición)
    if (appState == STATE_RUNNING) appState = STATE_PAUSED;
    cuadriculaWebActualizada = true;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void manejarLimpiar() {
  limpiarCuadricula(currentGrid);
  generacionActual = 0;
  poblacionActual = 0;
  appState = STATE_PAUSED;
  cuadriculaWebActualizada = true;
  server.send(200, "application/json", "{\"ok\":true}");
}

void manejarAleatorio() {
  aleatorizarCuadricula();
  generacionActual = 0;
  poblacionActual = contarPoblacion();
  cuadriculaWebActualizada = true;
  server.send(200, "application/json", "{\"ok\":true}");
}

void manejarPausar() {
  appState = STATE_PAUSED;
  server.send(200, "application/json", "{\"ok\":true}");
}

void manejarReanudar() {
  appState = STATE_RUNNING;
  server.send(200, "application/json", "{\"ok\":true}");
}

void manejarPaso() {
  if (appState == STATE_PAUSED) {
    memcpy(prevGrid, currentGrid, GRID_BYTES);
    calcularSiguienteGeneracion();
    uint8_t* temp = currentGrid;
    currentGrid = nextGrid;
    nextGrid = temp;
    generacionActual++;
    poblacionActual = contarPoblacion();
    cuadriculaWebActualizada = true;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void manejarVelocidad() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    velocidadSim = doc["speed"] | 150;
    velocidadSim = constrain(velocidadSim, 20, 2000);
    guardarConfiguracion();
  }
  server.send(200, "application/json", "{\"speed\":" + String(velocidadSim) + "}");
}

void manejarTema() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    temaActual = doc["theme"] | 0;
    temaActual = temaActual % NUM_THEMES;
    guardarConfiguracion();
    if (pantallaEncendida) {
      tft.fillScreen(themes[temaActual].muerta);
      dibujarCuadricula(true);
      dibujarBarraEstado();
    }
  }
  server.send(200, "application/json", "{\"theme\":" + String(temaActual) + "}");
}

void manejarPatron() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, server.arg("plain"));
    int tipo = doc["type"] | 0;
    int cx = doc["col"] | (GRID_COLS / 2);
    int cy = doc["row"] | (GRID_ROWS / 2);
    insertarPatron(tipo, cx, cy);
    poblacionActual = contarPoblacion();
    appState = STATE_PAUSED;
    cuadriculaWebActualizada = true;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void manejarEstadisticas() {
  String json = "{";
  json += "\"generation\":" + String(generacionActual);
  json += ",\"population\":" + String(poblacionActual);
  json += ",\"maxPopulation\":" + String(poblacionMaxima);
  json += ",\"totalGames\":" + String(partidasTotales);
  json += ",\"totalGenerations\":" + String(generacionesTotales);
  json += ",\"running\":" + String(appState == STATE_RUNNING ? "true" : "false");
  json += ",\"screenOn\":" + String(pantallaEncendida ? "true" : "false");
  json += ",\"speed\":" + String(velocidadSim);
  json += ",\"theme\":" + String(temaActual);
  json += ",\"freeHeap\":" + String(ESP.getFreeHeap());
  json += ",\"uptime\":" + String(millis() / 1000);
  json += ",\"wifiConnected\":" + String(wifiConectado ? "true" : "false");
  json += ",\"apMode\":" + String(modoAP ? "true" : "false");
  if (wifiConectado) {
    json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += ",\"rssi\":" + String(WiFi.RSSI());
  }
  json += ",\"gridCols\":" + String(GRID_COLS);
  json += ",\"gridRows\":" + String(GRID_ROWS);
  json += ",\"fwVersion\":\"" + String(FW_VERSION) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void manejarConfigWiFi() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<512> doc;
    deserializeJson(doc, server.arg("plain"));
    wifiSSID = doc["ssid"].as<String>();
    wifiPass = doc["pass"].as<String>();
    if (doc.containsKey("name")) {
      nombreDispositivo = doc["name"].as<String>();
    }
    guardarConfiguracion();
    server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Reiniciando...\"}");
    delay(1000);
    ESP.restart();
  }
  server.send(400, "application/json", "{\"error\":\"sin configuracion\"}");
}

void manejarEscaneoWiFi() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i));
    json += ",\"enc\":" + String(WiFi.encryptionType(i) != ENC_TYPE_NONE ? "true" : "false") + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void manejarProgramacion() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, server.arg("plain"));
    programacionActivada = doc["enabled"] | false;
    horaEncendido = doc["on"] | 7;
    horaApagado = doc["off"] | 23;
    guardarConfiguracion();
  }
  server.send(200, "application/json", 
    "{\"enabled\":" + String(programacionActivada ? "true" : "false") +
    ",\"on\":" + String(horaEncendido) +
    ",\"off\":" + String(horaApagado) + "}");
}

void manejarObtenerProgramacion() {
  server.send(200, "application/json",
    "{\"enabled\":" + String(programacionActivada ? "true" : "false") +
    ",\"on\":" + String(horaEncendido) +
    ",\"off\":" + String(horaApagado) + "}");
}

void manejarAlternarPantalla() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    pantallaEncendida = doc["on"] | true;
    if (pantallaEncendida) {
      establecerRetroiluminacion(brillo);
      tft.fillScreen(themes[temaActual].muerta);
      dibujarCuadricula(true);
      dibujarBarraEstado();
    } else {
      establecerRetroiluminacion(0);
    }
  }
  server.send(200, "application/json", "{\"screenOn\":" + String(pantallaEncendida ? "true" : "false") + "}");
}

void manejarBrillo() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    brillo = doc["brightness"] | 100;
    brillo = constrain(brillo, 0, 100);
    if (pantallaEncendida) {
      establecerRetroiluminacion(brillo);
    }
    guardarConfiguracion();
  }
  server.send(200, "application/json", "{\"brightness\":" + String(brillo) + "}");
}

void manejarObtenerConfig() {
  String json = "{";
  json += "\"speed\":" + String(velocidadSim);
  json += ",\"theme\":" + String(temaActual);
  json += ",\"scheduleEnabled\":" + String(programacionActivada ? "true" : "false");
  json += ",\"scheduleOn\":" + String(horaEncendido);
  json += ",\"scheduleOff\":" + String(horaApagado);
  json += ",\"brightness\":" + String(brillo);
  json += ",\"deviceName\":\"" + nombreDispositivo + "\"";
  json += ",\"themes\":[";
  for (int i = 0; i < NUM_THEMES; i++) {
    if (i > 0) json += ",";
    json += "\"" + String(themes[i].nombre) + "\"";
  }
  json += "]";
  json += "}";
  server.send(200, "application/json", json);
}

// ═════════════════════════════════════════════════════════════
//  OPERACIONES DE CUADRÍCULA
// ═════════════════════════════════════════════════════════════
inline bool obtenerCelula(uint8_t* grid, int col, int fila) {
  if (col < 0 || col >= GRID_COLS || fila < 0 || fila >= GRID_ROWS) return false;
  int idx = fila * GRID_COLS + col;
  return (grid[idx / 8] >> (idx % 8)) & 1;
}

inline void establecerCelula(uint8_t* grid, int col, int fila, bool val) {
  if (col < 0 || col >= GRID_COLS || fila < 0 || fila >= GRID_ROWS) return;
  int idx = fila * GRID_COLS + col;
  if (val) {
    grid[idx / 8] |= (1 << (idx % 8));
  } else {
    grid[idx / 8] &= ~(1 << (idx % 8));
  }
}

void limpiarCuadricula(uint8_t* grid) {
  memset(grid, 0, GRID_BYTES);
}

void aleatorizarCuadricula() {
  limpiarCuadricula(currentGrid);
  // Usar una densidad inicial más interesante
  for (int f = 0; f < GRID_ROWS; f++) {
    for (int c = 0; c < GRID_COLS; c++) {
      if (random(100) < 25) { // 25% de densidad
        establecerCelula(currentGrid, c, f, true);
      }
    }
  }
}

int contarVecinos(uint8_t* grid, int col, int fila) {
  int cuenta = 0;
  for (int df = -1; df <= 1; df++) {
    for (int dc = -1; dc <= 1; dc++) {
      if (df == 0 && dc == 0) continue;
      // Envoltura toroidal
      int nc = (col + dc + GRID_COLS) % GRID_COLS;
      int nf = (fila + df + GRID_ROWS) % GRID_ROWS;
      if (obtenerCelula(grid, nc, nf)) cuenta++;
    }
  }
  return cuenta;
}

void calcularSiguienteGeneracion() {
  limpiarCuadricula(nextGrid);
  for (int f = 0; f < GRID_ROWS; f++) {
    for (int c = 0; c < GRID_COLS; c++) {
      int vecinos = contarVecinos(currentGrid, c, f);
      bool viva = obtenerCelula(currentGrid, c, f);
      
      if (viva && (vecinos == 2 || vecinos == 3)) {
        establecerCelula(nextGrid, c, f, true);
      } else if (!viva && vecinos == 3) {
        establecerCelula(nextGrid, c, f, true);
      }
    }
    yield(); // Alimentar watchdog en cada fila
  }
}

uint16_t contarPoblacion() {
  uint16_t cuenta = 0;
  for (int i = 0; i < GRID_BYTES; i++) {
    uint8_t b = currentGrid[i];
    // Conteo de bits de Brian Kernighan
    while (b) {
      cuenta++;
      b &= b - 1;
    }
  }
  return cuenta;
}

// ═════════════════════════════════════════════════════════════
//  DIBUJADO
// ═════════════════════════════════════════════════════════════
void dibujarCuadricula(bool redibujadoCompleto) {
  const TemaColor& t = themes[temaActual];
  
  if (redibujadoCompleto) {
    for (int f = 0; f < GRID_ROWS; f++) {
      for (int c = 0; c < GRID_COLS; c++) {
        bool viva = obtenerCelula(currentGrid, c, f);
        if (viva) {
          tft.fillRect(c * CELL_SIZE, f * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, t.viva);
        }
      }
      yield();
    }
  } else {
    // Solo dibujar células que cambiaron
    for (int f = 0; f < GRID_ROWS; f++) {
      for (int c = 0; c < GRID_COLS; c++) {
        bool estabaViva = obtenerCelula(prevGrid, c, f);
        bool estaViva = obtenerCelula(currentGrid, c, f);
        
        if (estabaViva != estaViva) {
          if (estaViva) {
            // Célula recién nacida
            tft.fillRect(c * CELL_SIZE, f * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, t.recienNacida);
          } else {
            // Célula muriendo - destello breve y luego muerta
            tft.fillRect(c * CELL_SIZE, f * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, t.muerta);
          }
        } else if (estaViva) {
          // Célula viva estable - color normal
          tft.fillRect(c * CELL_SIZE, f * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, t.viva);
        }
      }
      yield();
    }
  }
}

void dibujarCelula(int col, int fila, bool viva, bool esNueva, bool estaMuriendo) {
  const TemaColor& t = themes[temaActual];
  uint16_t color;
  if (viva) {
    color = esNueva ? t.recienNacida : t.viva;
  } else {
    color = estaMuriendo ? t.muriendo : t.muerta;
  }
  tft.fillRect(col * CELL_SIZE, fila * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, color);
}

void dibujarBarraEstado() {
  // Barra de estado: 320x20px, fuente tamaño 1 = 6x8px por carácter
  // Distribución: [WiFi·][MENU]  Gen:X Pob:X Max:X  [EJEC/PAUSA]
  //               |--- izquierda ---|--- centro ---|--- derecha ---|
  const TemaColor& t = themes[temaActual];
  int y = STATUS_BAR_Y;
  
  // Limpiar toda la barra
  tft.fillRect(0, y, SCREEN_WIDTH, 20, t.fondoEstado);
  tft.setTextSize(1);
  
  // ── IZQUIERDA: Indicador WiFi (círculo) + botón MENU ──
  // Círculo WiFi en x=5, centrado verticalmente en la barra (y+10)
  uint16_t colorWifi = TFT_RED;
  if (wifiConectado) colorWifi = TFT_GREEN;
  else if (modoAP) colorWifi = TFT_YELLOW;
  tft.fillCircle(6, y + 10, 3, colorWifi);
  
  // Texto "MENU" justo después del círculo WiFi
  tft.setTextColor(t.textoEstado, t.fondoEstado);
  tft.setCursor(14, y + 6);
  tft.print("MENU");
  // "MENU" ocupa 4×6 = 24px, termina en x=38
  // Zona táctil MENU: x < 50
  
  // ── CENTRO: Estadísticas compactas ──
  // Formato compacto para que quepa en el espacio central (x=50 a x=255)
  char buf[50];
  sprintf(buf, "G:%lu P:%d M:%lu", 
          generacionActual, poblacionActual, poblacionMaxima);
  tft.setTextColor(t.textoEstado, t.fondoEstado);
  tft.setCursor(50, y + 6);
  tft.print(buf);
  
  // ── DERECHA: Estado de ejecución ──
  // "EJEC" = 4 chars × 6px = 24px; "PAUSA" = 5 chars × 6px = 30px
  const char* textoEstadoStr;
  switch (appState) {
    case STATE_RUNNING: textoEstadoStr = "EJEC";  break;
    case STATE_PAUSED:  textoEstadoStr = "PAUSA"; break;
    case STATE_MENU:    textoEstadoStr = "MENU";  break;
    default:            textoEstadoStr = "...";   break;
  }
  // Calcular ancho del texto para alinear a la derecha con margen de 4px
  int anchoTextoEstado = strlen(textoEstadoStr) * 6; // 6px por carácter a tamaño 1
  int xEstado = SCREEN_WIDTH - anchoTextoEstado - 4;
  
  // Fondo resaltado para el indicador de estado
  uint16_t colorFondoEstado = (appState == STATE_RUNNING) ? 0x03E0 : 0x7800; // verde oscuro / rojo oscuro
  tft.fillRoundRect(xEstado - 3, y + 2, anchoTextoEstado + 6, 16, 3, colorFondoEstado);
  tft.setTextColor(TFT_WHITE, colorFondoEstado);
  tft.setCursor(xEstado, y + 6);
  tft.print(textoEstadoStr);
  // Zona táctil estado: x > 255
}

// ═════════════════════════════════════════════════════════════
//  MANEJADOR DE WEBSOCKET
// ═════════════════════════════════════════════════════════════
void manejarEventoWebSocket(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Cliente #%u desconectado\n", num);
      break;
    case WStype_CONNECTED:
      {
        Serial.printf("[WS] Cliente #%u conectado\n", num);
        // Enviar estado actual al nuevo cliente
        String msg = "{\"g\":" + String(generacionActual) +
                     ",\"p\":" + String(poblacionActual) +
                     ",\"m\":" + String(poblacionMaxima) +
                     ",\"r\":" + String(appState == STATE_RUNNING ? 1 : 0) +
                     ",\"t\":" + String(partidasTotales) + "}";
        webSocket.sendTXT(num, msg);
      }
      break;
    case WStype_TEXT:
      {
        // Recibir comandos desde el cliente WS
        String cmd = String((char*)payload);
        if (cmd == "pause") {
          appState = STATE_PAUSED;
          dibujarBarraEstado();
        } else if (cmd == "resume") {
          appState = STATE_RUNNING;
          dibujarBarraEstado();
        } else if (cmd == "step") {
          if (appState == STATE_RUNNING) appState = STATE_PAUSED;
          // Se ejecutará un paso en el próximo ciclo
        }
      }
      break;
    default:
      break;
  }
}

// ═════════════════════════════════════════════════════════════
//  MANEJO TÁCTIL
// ═════════════════════════════════════════════════════════════
void manejarTactil() {
  uint16_t tx, ty;
  bool tocado = tft.getTouch(&tx, &ty, TOUCH_THRESHOLD);
  
  if (!tocado) return;
  
  // Anti-rebote
  if (millis() - ultimoTiempoTactil < 100) return;
  ultimoTiempoTactil = millis();
  
  // Si la pantalla está apagada, encenderla al tocar y volver a la configuración
  if (!pantallaEncendida) {
    pantallaEncendida = true;
    establecerRetroiluminacion(brillo);
    appState = STATE_CONFIG;
    dibujarConfigGeneral();
    return;
  }
  
  // Mapear coordenadas táctiles a la pantalla
  // El panel táctil tiene el eje X invertido respecto a la pantalla
  int sx = (SCREEN_WIDTH - 1) - tx;
  int sy = ty;
  
  switch (appState) {
    case STATE_RUNNING:
    case STATE_PAUSED:
      // Verificar si se toca la barra de estado
      if (sy >= STATUS_BAR_Y) {
        if (sx < 50) {
          // Zona izquierda: botón MENU presionado
          appState = STATE_MENU;
          dibujarMenuPrincipal();
          return;
        }
        if (sx > 255) {
          // Zona derecha: alternar pausa/ejecución
          if (appState == STATE_RUNNING) {
            appState = STATE_PAUSED;
          } else {
            appState = STATE_RUNNING;
          }
          dibujarBarraEstado();
          return;
        }
        // Zona central (estadísticas): no hace nada
        return;
      }
      
      // Alternar células
      {
        int col = sx / CELL_SIZE;
        int fila = sy / CELL_SIZE;
        if (col >= 0 && col < GRID_COLS && fila >= 0 && fila < GRID_ROWS) {
          bool actual = obtenerCelula(currentGrid, col, fila);
          establecerCelula(currentGrid, col, fila, !actual);
          dibujarCelula(col, fila, !actual);
          poblacionActual = contarPoblacion();
        }
      }
      break;
      
    case STATE_MENU:
      manejarTactilMenu(sx, sy);
      break;
      
    case STATE_WIFI_CONFIG:
      manejarTactilConfigWiFi(sx, sy);
      break;
      
    case STATE_PATTERN_SELECT:
      manejarTactilPatron(sx, sy);
      break;
      
    case STATE_CONFIG:
      manejarTactilConfig(sx, sy);
      break;
      
    case STATE_THEME_SELECT:
      manejarTactilTema(sx, sy);
      break;
  }
}

// ═════════════════════════════════════════════════════════════
//  MENÚS
// ═════════════════════════════════════════════════════════════
void dibujarMenuPrincipal() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(themes[temaActual].viva);
  tft.setCursor(80, 5);
  tft.println("= MENU =");
  
  // Mostrar IP debajo del título en pequeño
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(80, 24);
  if (wifiConectado) {
    tft.print("IP: " + WiFi.localIP().toString());
  } else if (modoAP) {
    tft.print("AP: " + WiFi.softAPIP().toString());
  }
  
  const char* elementos[] = {
    "Reanudar Juego",
    "Nueva Cuadricula Aleatoria",
    "Limpiar Cuadricula",
    "Insertar Patron",
    "Cambiar Tema",
    "Ajustes WiFi",
    "Configuracion",
    "Borrar Estadisticas",
    "Volver"
  };
  cantidadElementosMenu = 9;
  
  // 9 elementos × 22px = 198px, empezando en y=36, termina en y=234
  tft.setTextSize(1);
  for (int i = 0; i < cantidadElementosMenu; i++) {
    int y = 36 + i * 22;
    // Color diferente para "Borrar Estadísticas" (peligro)
    uint16_t colorFondo = 0x2104;
    if (i == 7) colorFondo = 0x4000; // rojo oscuro para borrar estadísticas
    tft.fillRoundRect(20, y, 280, 18, 3, colorFondo);
    tft.setTextColor(TFT_WHITE);
    if (i == 7) tft.setTextColor(0xFBE0); // amarillo para la opción peligrosa
    tft.setCursor(28, y + 5);
    tft.print(String(i + 1) + ". " + elementos[i]);
  }
}

void manejarTactilMenu(int x, int y) {
  int elemento = (y - 36) / 22;
  if (elemento < 0 || elemento >= cantidadElementosMenu) return;
  
  switch (elemento) {
    case 0: // Reanudar
      appState = STATE_RUNNING;
      tft.fillScreen(themes[temaActual].muerta);
      dibujarCuadricula(true);
      dibujarBarraEstado();
      break;
      
    case 1: // Aleatorio
      aleatorizarCuadricula();
      generacionActual = 0;
      partidasTotales++;
      poblacionMaxima = 0;
      appState = STATE_RUNNING;
      tft.fillScreen(themes[temaActual].muerta);
      dibujarCuadricula(true);
      dibujarBarraEstado();
      break;
      
    case 2: // Limpiar
      limpiarCuadricula(currentGrid);
      generacionActual = 0;
      poblacionActual = 0;
      appState = STATE_PAUSED;
      tft.fillScreen(themes[temaActual].muerta);
      dibujarCuadricula(true);
      dibujarBarraEstado();
      break;
      
    case 3: // Patrones
      appState = STATE_PATTERN_SELECT;
      dibujarSeleccionPatron();
      break;
      
    case 4: // Tema
      paginaTemas = 0;
      appState = STATE_THEME_SELECT;
      dibujarSeleccionTema();
      break;
      
    case 5: // WiFi
      appState = STATE_WIFI_CONFIG;
      dibujarConfigWiFi();
      break;
      
    case 6: // Configuración
      appState = STATE_CONFIG;
      dibujarConfigGeneral();
      break;
      
    case 7: { // Borrar Estadísticas
      // Poner a cero todos los contadores de estadísticas
      partidasTotales = 0;
      generacionesTotales = 0;
      generacionActual = 0;
      poblacionMaxima = 0;
      contadorEstancamiento = 0;
      guardarEstadisticas();
      
      // Mostrar confirmación visual
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(TFT_GREEN);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("Estadisticas", 160, 100);
      tft.drawString("borradas!", 160, 125);
      tft.setTextDatum(TL_DATUM);
      delay(1500);
      
      dibujarMenuPrincipal();
      break;
    }
      
    case 8: // Volver
      appState = STATE_RUNNING;
      tft.fillScreen(themes[temaActual].muerta);
      dibujarCuadricula(true);
      dibujarBarraEstado();
      break;
  }
}

void dibujarConfigWiFi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(themes[temaActual].viva);
  tft.setCursor(50, 10);
  tft.println("Config WiFi");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  
  tft.setCursor(10, 40);
  tft.print("Estado: ");
  if (wifiConectado) {
    tft.setTextColor(TFT_GREEN);
    tft.println("Conectado");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 55);
    tft.print("IP: ");
    tft.println(WiFi.localIP().toString());
    tft.setCursor(10, 70);
    tft.print("RSSI: ");
    tft.print(WiFi.RSSI());
    tft.println(" dBm");
  } else {
    tft.setTextColor(TFT_YELLOW);
    tft.println("Modo AP");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 55);
    tft.print("IP AP: ");
    tft.println(WiFi.softAPIP().toString());
  }
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 95);
  tft.println("Usa la interfaz web para configurar WiFi.");
  tft.setCursor(10, 110);
  tft.println("Conectate al AP y visita la IP de arriba.");
  
  // Botones
  tft.fillRoundRect(20, 150, 130, 30, 5, 0x03E0);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(35, 160);
  tft.print("Iniciar Modo AP");
  
  tft.fillRoundRect(170, 150, 130, 30, 5, 0x001F);
  tft.setCursor(200, 160);
  tft.print("Volver");
  
  tft.fillRoundRect(20, 195, 280, 30, 5, 0x7800);
  tft.setCursor(80, 205);
  tft.print("Reiniciar Dispositivo");
}

void manejarTactilConfigWiFi(int x, int y) {
  if (y >= 150 && y <= 180) {
    if (x < 160) {
      // Iniciar Modo AP
      iniciarModoAP();
      dibujarConfigWiFi();
    } else {
      // Volver
      appState = STATE_MENU;
      dibujarMenuPrincipal();
    }
  } else if (y >= 195 && y <= 225) {
    // Reiniciar
    ESP.restart();
  }
}

void dibujarSeleccionPatron() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(themes[temaActual].viva);
  tft.setCursor(40, 10);
  tft.println("Insertar Patron");
  
  const char* patrones[] = {
    "Planeador (Glider)",
    "LWSS (Nave Espacial Ligera)",
    "Pulsar",
    "Canon de Planeadores de Gosper",
    "R-Pentomino",
    "Bellota (Acorn)",
    "Diehard",
    "Volver"
  };
  cantidadElementosMenu = 8;
  
  tft.setTextSize(1);
  for (int i = 0; i < cantidadElementosMenu; i++) {
    int y = 40 + i * 24;
    tft.fillRoundRect(20, y, 280, 20, 4, 0x2104);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(30, y + 4);
    tft.print(String(i + 1) + ". " + patrones[i]);
  }
}

void manejarTactilPatron(int x, int y) {
  int elemento = (y - 40) / 24;
  if (elemento < 0 || elemento >= cantidadElementosMenu) return;
  
  if (elemento == 7) { // Volver
    appState = STATE_MENU;
    dibujarMenuPrincipal();
    return;
  }
  
  // Insertar patrón en el centro
  insertarPatron(elemento, GRID_COLS / 2, GRID_ROWS / 2);
  poblacionActual = contarPoblacion();
  appState = STATE_RUNNING;
  tft.fillScreen(themes[temaActual].muerta);
  dibujarCuadricula(true);
  dibujarBarraEstado();
}

// ═════════════════════════════════════════════════════════════
//  PANTALLA DE SELECCIÓN DE TEMA
// ═════════════════════════════════════════════════════════════

// 20 temas → 2 columnas × 5 filas = 10 por página, 2 páginas
#define TEMAS_POR_PAGINA 10
#define TEMAS_COLUMNAS   2
#define TEMAS_FILAS      5

void dibujarSeleccionTema() {
  tft.fillScreen(TFT_BLACK);
  
  // ── Título ──
  tft.setTextSize(2);
  tft.setTextColor(0x07FF); // Cian
  tft.setCursor(30, 4);
  tft.print("SELECCIONAR TEMA");
  tft.setTextSize(1);
  
  // Indicador de página
  int totalPaginas = (NUM_THEMES + TEMAS_POR_PAGINA - 1) / TEMAS_POR_PAGINA;
  char pagBuf[16];
  sprintf(pagBuf, "Pag %d/%d", paginaTemas + 1, totalPaginas);
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(260, 10);
  tft.print(pagBuf);
  
  // ── Cuadrícula de temas ──
  // Cada casilla: 150×34px, 2 columnas con separación
  // Columna 0: x=5, Columna 1: x=165
  // Filas empiezan en y=26, separación de 38px
  const int tileW = 150;
  const int tileH = 34;
  const int colX[2] = {5, 165};
  const int startY = 26;
  const int rowH = 38;
  
  int inicio = paginaTemas * TEMAS_POR_PAGINA;
  int fin = inicio + TEMAS_POR_PAGINA;
  if (fin > (int)NUM_THEMES) fin = NUM_THEMES;
  
  for (int i = inicio; i < fin; i++) {
    int idx = i - inicio;
    int col = idx % TEMAS_COLUMNAS;
    int fila = idx / TEMAS_COLUMNAS;
    int x = colX[col];
    int y = startY + fila * rowH;
    
    const TemaColor& t = themes[i];
    
    // Fondo de la casilla (color muerta del tema)
    tft.fillRoundRect(x, y, tileW, tileH, 4, t.muerta);
    
    // Borde: resaltado si es el tema activo
    if (i == temaActual) {
      tft.drawRoundRect(x, y, tileW, tileH, 4, TFT_WHITE);
      tft.drawRoundRect(x + 1, y + 1, tileW - 2, tileH - 2, 3, TFT_WHITE);
    } else {
      tft.drawRoundRect(x, y, tileW, tileH, 4, 0x4208);
    }
    
    // Franja de previsualización: 3 rectángulos simulando células vivas
    int prevX = x + 4;
    int prevY = y + 4;
    // Células vivas
    for (int cx = 0; cx < 5; cx++) {
      tft.fillRect(prevX + cx * 7, prevY, 5, 5, t.viva);
    }
    // Célula recién nacida
    tft.fillRect(prevX + 35, prevY, 5, 5, t.recienNacida);
    // Célula muriendo
    tft.fillRect(prevX + 42, prevY, 5, 5, t.muriendo);
    
    // Nombre del tema
    tft.setTextColor(t.textoEstado, t.muerta);
    tft.setCursor(x + 4, y + 14);
    tft.setTextSize(1);
    tft.print(t.nombre);
    
    // Indicador de selección actual
    if (i == temaActual) {
      tft.setTextColor(TFT_WHITE, t.muerta);
      tft.setCursor(x + 4, y + 24);
      tft.print("< actual >");
    }
  }
  
  // ── Botones de navegación en la parte inferior ──
  int btnY = 218;
  
  // Botón "Anterior" (solo si no estamos en la primera página)
  if (paginaTemas > 0) {
    dibujarBotonConfig(10, btnY, 80, 20, "<< Ant.", 0x2104, TFT_WHITE);
  }
  
  // Botón "Volver" centrado
  dibujarBotonConfig(115, btnY, 90, 20, "Volver", 0x0010, TFT_CYAN);
  
  // Botón "Siguiente" (solo si hay más páginas)
  if (fin < (int)NUM_THEMES) {
    dibujarBotonConfig(230, btnY, 80, 20, "Sig. >>", 0x2104, TFT_WHITE);
  }
}

void manejarTactilTema(int x, int y) {
  // ── Detección de toque en las casillas de temas ──
  const int tileW = 150;
  const int tileH = 34;
  const int colX[2] = {5, 165};
  const int startY = 26;
  const int rowH = 38;
  
  // Verificar si se tocó una casilla de tema
  if (y >= startY && y < startY + TEMAS_FILAS * rowH) {
    int fila = (y - startY) / rowH;
    int col = -1;
    if (x >= colX[0] && x < colX[0] + tileW) col = 0;
    else if (x >= colX[1] && x < colX[1] + tileW) col = 1;
    
    if (col >= 0 && fila >= 0 && fila < TEMAS_FILAS) {
      int idx = paginaTemas * TEMAS_POR_PAGINA + fila * TEMAS_COLUMNAS + col;
      if (idx >= 0 && idx < (int)NUM_THEMES) {
        temaActual = idx;
        guardarConfiguracion();
        dibujarSeleccionTema(); // Redibujar con selección actualizada
        return;
      }
    }
  }
  
  // ── Botones de navegación (y >= 218) ──
  if (y >= 218 && y < 240) {
    int totalPaginas = (NUM_THEMES + TEMAS_POR_PAGINA - 1) / TEMAS_POR_PAGINA;
    
    if (x >= 10 && x < 90 && paginaTemas > 0) {
      // Anterior
      paginaTemas--;
      dibujarSeleccionTema();
      return;
    }
    
    if (x >= 115 && x < 205) {
      // Volver al menú
      appState = STATE_MENU;
      dibujarMenuPrincipal();
      return;
    }
    
    if (x >= 230 && x < 310 && paginaTemas < totalPaginas - 1) {
      // Siguiente
      paginaTemas++;
      dibujarSeleccionTema();
      return;
    }
  }
}

// ═════════════════════════════════════════════════════════════
//  PANTALLA DE CONFIGURACIÓN GENERAL
// ═════════════════════════════════════════════════════════════

// Función auxiliar para aplicar la zona horaria al NTP
void aplicarZonaHoraria() {
  configTime(zonaHoraria * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

// Dibuja un botón táctil rectangular con texto centrado
void dibujarBotonConfig(int x, int y, int w, int h, const char* texto, uint16_t colorFondo, uint16_t colorTexto) {
  tft.fillRoundRect(x, y, w, h, 3, colorFondo);
  tft.drawRoundRect(x, y, w, h, 3, 0x4208);
  tft.setTextColor(colorTexto, colorFondo);
  // Centrar texto horizontalmente en el botón
  int anchoTexto = strlen(texto) * 6;
  tft.setCursor(x + (w - anchoTexto) / 2, y + (h - 8) / 2);
  tft.print(texto);
}

void dibujarConfigGeneral() {
  const TemaColor& t = themes[temaActual];
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  
  // ── Título ──
  tft.setTextSize(2);
  tft.setTextColor(t.viva);
  tft.setCursor(50, 4);
  tft.print("CONFIGURACION");
  tft.setTextSize(1);
  
  // ── Hora actual ──  (fila y=24)
  tft.setTextColor(TFT_DARKGREY);
  tft.setCursor(10, 26);
  time_t ahora = time(nullptr);
  struct tm* info = localtime(&ahora);
  if (info->tm_year > 100) {
    char horaStr[32];
    sprintf(horaStr, "Hora: %02d:%02d:%02d  (UTC%+d)", 
            info->tm_hour, info->tm_min, info->tm_sec, zonaHoraria);
    tft.print(horaStr);
  } else {
    tft.print("Hora: Sin sincronizar (NTP)");
  }
  
  // Posiciones Y para cada fila de configuración
  // Cada fila: etiqueta a la izquierda, botones [-][valor][+] a la derecha
  // Botones: [-] en x=200, valor en x=240, [+] en x=280
  const int xLabel = 10;
  const int xMinus = 195;
  const int xValue = 228;
  const int xPlus  = 275;
  const int btnW   = 30;
  const int btnH   = 18;
  
  // ── Fila 1: Velocidad (y=40) ──
  int yRow = 40;
  tft.setTextColor(t.textoEstado);
  tft.setCursor(xLabel, yRow + 5);
  tft.print("Velocidad sim:");
  dibujarBotonConfig(xMinus, yRow, btnW, btnH, "-", 0x2104, TFT_WHITE);
  char valBuf[16];
  sprintf(valBuf, "%dms", velocidadSim);
  tft.setTextColor(t.viva, TFT_BLACK);
  int anchoVal = strlen(valBuf) * 6;
  tft.setCursor(xValue + (44 - anchoVal) / 2, yRow + 5);
  tft.print(valBuf);
  dibujarBotonConfig(xPlus, yRow, btnW, btnH, "+", 0x2104, TFT_WHITE);
  
  // ── Fila 2: Brillo (y=64) ──
  yRow = 64;
  tft.setTextColor(t.textoEstado);
  tft.setCursor(xLabel, yRow + 5);
  tft.print("Brillo pantalla:");
  dibujarBotonConfig(xMinus, yRow, btnW, btnH, "-", 0x2104, TFT_WHITE);
  sprintf(valBuf, "%d%%", brillo);
  tft.setTextColor(t.viva, TFT_BLACK);
  anchoVal = strlen(valBuf) * 6;
  tft.setCursor(xValue + (44 - anchoVal) / 2, yRow + 5);
  tft.print(valBuf);
  dibujarBotonConfig(xPlus, yRow, btnW, btnH, "+", 0x2104, TFT_WHITE);
  
  // ── Separador ──
  tft.drawFastHLine(10, 87, 300, 0x2104);
  
  // ── Fila 3: Programación encendido/apagado (y=92) ──
  yRow = 92;
  tft.setTextColor(t.textoEstado);
  tft.setCursor(xLabel, yRow + 5);
  tft.print("Auto encendido/apagado:");
  if (programacionActivada) {
    dibujarBotonConfig(xPlus, yRow, btnW, btnH, "SI", 0x03E0, TFT_WHITE);
  } else {
    dibujarBotonConfig(xPlus, yRow, btnW, btnH, "NO", 0x7800, TFT_WHITE);
  }
  
  // ── Fila 4: Hora encendido (y=114) ──
  yRow = 114;
  tft.setTextColor(programacionActivada ? t.textoEstado : 0x4208);
  tft.setCursor(xLabel + 10, yRow + 5);
  tft.print("Encender a las:");
  dibujarBotonConfig(xMinus, yRow, btnW, btnH, "-", programacionActivada ? 0x2104 : 0x1082, TFT_WHITE);
  sprintf(valBuf, "%02d:00", horaEncendido);
  tft.setTextColor(programacionActivada ? t.viva : 0x4208, TFT_BLACK);
  tft.setCursor(xValue + 4, yRow + 5);
  tft.print(valBuf);
  dibujarBotonConfig(xPlus, yRow, btnW, btnH, "+", programacionActivada ? 0x2104 : 0x1082, TFT_WHITE);
  
  // ── Fila 5: Hora apagado (y=136) ──
  yRow = 136;
  tft.setTextColor(programacionActivada ? t.textoEstado : 0x4208);
  tft.setCursor(xLabel + 10, yRow + 5);
  tft.print("Apagar a las:");
  dibujarBotonConfig(xMinus, yRow, btnW, btnH, "-", programacionActivada ? 0x2104 : 0x1082, TFT_WHITE);
  sprintf(valBuf, "%02d:00", horaApagado);
  tft.setTextColor(programacionActivada ? t.viva : 0x4208, TFT_BLACK);
  tft.setCursor(xValue + 4, yRow + 5);
  tft.print(valBuf);
  dibujarBotonConfig(xPlus, yRow, btnW, btnH, "+", programacionActivada ? 0x2104 : 0x1082, TFT_WHITE);
  
  // ── Separador ──
  tft.drawFastHLine(10, 159, 300, 0x2104);
  
  // ── Fila 6: Zona horaria (y=164) ──
  yRow = 164;
  tft.setTextColor(t.textoEstado);
  tft.setCursor(xLabel, yRow + 5);
  tft.print("Zona horaria:");
  dibujarBotonConfig(xMinus, yRow, btnW, btnH, "-", 0x2104, TFT_WHITE);
  sprintf(valBuf, "UTC%+d", zonaHoraria);
  tft.setTextColor(t.viva, TFT_BLACK);
  anchoVal = strlen(valBuf) * 6;
  tft.setCursor(xValue + (44 - anchoVal) / 2, yRow + 5);
  tft.print(valBuf);
  dibujarBotonConfig(xPlus, yRow, btnW, btnH, "+", 0x2104, TFT_WHITE);
  
  // ── Fila 7: Control de pantalla (y=190) ──
  yRow = 190;
  tft.setTextColor(t.textoEstado);
  tft.setCursor(xLabel, yRow + 5);
  tft.print("Pantalla:");
  dibujarBotonConfig(195, yRow, 50, btnH, "ON", pantallaEncendida ? 0x03E0 : 0x2104, TFT_WHITE);
  dibujarBotonConfig(255, yRow, 50, btnH, "OFF", !pantallaEncendida ? 0x7800 : 0x2104, TFT_WHITE);
  
  // ── Botón Guardar y Volver (y=214) ──
  dibujarBotonConfig(40, 214, 240, 22, "Guardar y Volver", 0x0010, TFT_CYAN);
}

void manejarTactilConfig(int x, int y) {
  // Definiciones de zonas táctiles (mismas posiciones que el dibujado)
  const int xMinus = 195;
  const int xPlus  = 275;
  const int btnW   = 30;
  const int btnH   = 18;
  bool cambio = false;
  
  // ── Fila 1: Velocidad (y=40) ──
  if (y >= 40 && y < 58) {
    if (x >= xMinus && x < xMinus + btnW) {
      // Disminuir velocidad (más rápido: menor ms)
      if (velocidadSim > 20) {
        velocidadSim = (velocidadSim <= 50) ? 20 : velocidadSim - 25;
      }
      cambio = true;
    } else if (x >= xPlus && x < xPlus + btnW) {
      // Aumentar velocidad (más lento: mayor ms)
      if (velocidadSim < 2000) {
        velocidadSim = (velocidadSim >= 1000) ? 2000 : velocidadSim + 25;
      }
      cambio = true;
    }
  }
  
  // ── Fila 2: Brillo (y=64) ──
  else if (y >= 64 && y < 82) {
    if (x >= xMinus && x < xMinus + btnW) {
      if (brillo >= 10) brillo -= 10;
      else brillo = 0;
      establecerRetroiluminacion(brillo);
      cambio = true;
    } else if (x >= xPlus && x < xPlus + btnW) {
      if (brillo <= 90) brillo += 10;
      else brillo = 100;
      establecerRetroiluminacion(brillo);
      cambio = true;
    }
  }
  
  // ── Fila 3: Programación ON/OFF (y=92) ──
  else if (y >= 92 && y < 110) {
    if (x >= xPlus && x < xPlus + btnW) {
      programacionActivada = !programacionActivada;
      cambio = true;
    }
  }
  
  // ── Fila 4: Hora encendido (y=114) ──
  else if (y >= 114 && y < 132 && programacionActivada) {
    if (x >= xMinus && x < xMinus + btnW) {
      horaEncendido = (horaEncendido == 0) ? 23 : horaEncendido - 1;
      cambio = true;
    } else if (x >= xPlus && x < xPlus + btnW) {
      horaEncendido = (horaEncendido == 23) ? 0 : horaEncendido + 1;
      cambio = true;
    }
  }
  
  // ── Fila 5: Hora apagado (y=136) ──
  else if (y >= 136 && y < 154 && programacionActivada) {
    if (x >= xMinus && x < xMinus + btnW) {
      horaApagado = (horaApagado == 0) ? 23 : horaApagado - 1;
      cambio = true;
    } else if (x >= xPlus && x < xPlus + btnW) {
      horaApagado = (horaApagado == 23) ? 0 : horaApagado + 1;
      cambio = true;
    }
  }
  
  // ── Fila 6: Zona horaria (y=164) ──
  else if (y >= 164 && y < 182) {
    if (x >= xMinus && x < xMinus + btnW) {
      if (zonaHoraria > -12) zonaHoraria--;
      aplicarZonaHoraria();
      cambio = true;
    } else if (x >= xPlus && x < xPlus + btnW) {
      if (zonaHoraria < 14) zonaHoraria++;
      aplicarZonaHoraria();
      cambio = true;
    }
  }
  
  // ── Fila 7: Control pantalla (y=190) ──
  else if (y >= 190 && y < 208) {
    if (x >= 195 && x < 245) {
      // Botón ON
      pantallaEncendida = true;
      establecerRetroiluminacion(brillo);
      cambio = true;
    } else if (x >= 255 && x < 305) {
      // Botón OFF
      pantallaEncendida = false;
      establecerRetroiluminacion(0);
      // Nota: la pantalla se apagará pero el usuario puede volver a encenderla
      // tocando la pantalla (aunque no se vea)
      cambio = true;
    }
  }
  
  // ── Botón Guardar y Volver (y=214) ──
  else if (y >= 214 && y < 236) {
    if (x >= 40 && x < 280) {
      guardarConfiguracion();
      appState = STATE_MENU;
      dibujarMenuPrincipal();
      return;
    }
  }
  
  // Redibujar la pantalla si hubo cambio
  if (cambio) {
    dibujarConfigGeneral();
  }
}

// ═════════════════════════════════════════════════════════════
//  PATRONES
// ═════════════════════════════════════════════════════════════
void colocarPatron(const int8_t pat[][2], int cantidad, int ox, int oy) {
  for (int i = 0; i < cantidad; i++) {
    establecerCelula(currentGrid, ox + pat[i][0], oy + pat[i][1], true);
  }
}

void insertarPatron(int tipo, int cx, int cy) {
  switch (tipo) {
    case 0: { // Planeador (Glider)
      const int8_t p[][2] = {{0,-1},{1,0},{-1,1},{0,1},{1,1}};
      colocarPatron(p, 5, cx, cy);
      break;
    }
    case 1: { // LWSS (Nave Espacial Ligera)
      const int8_t p[][2] = {{0,-2},{3,-2},{4,-1},{0,0},{4,0},{1,1},{2,1},{3,1},{4,1}};
      colocarPatron(p, 9, cx, cy);
      break;
    }
    case 2: { // Pulsar
      // El Pulsar es simétrico, definir un cuadrante
      const int8_t qx[] = {2,3,4, 6,6,6, 2,3,4};
      const int8_t qy[] = {1,1,1, 2,3,4, 6,6,6};
      for (int i = 0; i < 9; i++) {
        establecerCelula(currentGrid, cx+qx[i], cy+qy[i], true);
        establecerCelula(currentGrid, cx-qx[i], cy+qy[i], true);
        establecerCelula(currentGrid, cx+qx[i], cy-qy[i], true);
        establecerCelula(currentGrid, cx-qx[i], cy-qy[i], true);
      }
      break;
    }
    case 3: { // Cañón de Planeadores de Gosper (Gosper Glider Gun)
      const int8_t p[][2] = {
        {0,4},{0,5},{1,4},{1,5},
        {10,4},{10,5},{10,6},{11,3},{11,7},{12,2},{12,8},{13,2},{13,8},
        {14,5},{15,3},{15,7},{16,4},{16,5},{16,6},{17,5},
        {20,2},{20,3},{20,4},{21,2},{21,3},{21,4},{22,1},{22,5},
        {24,0},{24,1},{24,5},{24,6},
        {34,2},{34,3},{35,2},{35,3}
      };
      colocarPatron(p, 36, cx - 17, cy - 4);
      break;
    }
    case 4: { // R-Pentominó
      const int8_t p[][2] = {{0,0},{1,0},{-1,1},{0,1},{0,-1}};
      colocarPatron(p, 5, cx, cy);
      break;
    }
    case 5: { // Bellota (Acorn)
      const int8_t p[][2] = {{-3,0},{-2,0},{-2,-2},{0,-1},{1,0},{2,0},{3,0}};
      colocarPatron(p, 7, cx, cy);
      break;
    }
    case 6: { // Diehard (Intransigente)
      const int8_t p[][2] = {{-3,1},{-2,1},{-2,0},{2,0},{3,0},{4,0},{3,-1}};
      colocarPatron(p, 7, cx, cy);
      break;
    }
  }
}

// ═════════════════════════════════════════════════════════════
//  VERIFICACIÓN DE ESTANCAMIENTO / EXTINCIÓN
// ═════════════════════════════════════════════════════════════
void verificarEstancamiento() {
  if (poblacionActual == 0) {
    // ¡Extinción!
    partidasTotales++;
    guardarEstadisticas();
    
    if (pantallaEncendida) {
      // Mostrar animación de extinción
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(3);
      tft.setTextColor(TFT_RED);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("EXTINCION!", 160, 80);
      tft.setTextSize(2);
      tft.setTextColor(TFT_YELLOW);
      char buf[40];
      sprintf(buf, "Partida #%lu", partidasTotales);
      tft.drawString(buf, 160, 120);
      sprintf(buf, "Duro %lu generaciones", generacionActual);
      tft.drawString(buf, 160, 150);
      tft.setTextDatum(TL_DATUM);
      delay(3000);
    } else {
      delay(1000);
    }
    
    // Reinicio automático con nueva cuadrícula aleatoria
    aleatorizarCuadricula();
    generacionActual = 0;
    poblacionMaxima = 0;
    poblacionActual = contarPoblacion();
    contadorEstancamiento = 0;
    
    if (pantallaEncendida) {
      tft.fillScreen(themes[temaActual].muerta);
      dibujarCuadricula(true);
      dibujarBarraEstado();
    }
    return;
  }
  
  // Detectar estancamiento (misma población por demasiado tiempo = probable patrón estable)
  if (poblacionActual == poblacionAnterior) {
    contadorEstancamiento++;
  } else {
    contadorEstancamiento = 0;
  }
  poblacionAnterior = poblacionActual;
  
  // Si estancado por más de 200 generaciones, añadir células aleatorias para animar las cosas
  if (contadorEstancamiento > 200) {
    contadorEstancamiento = 0;
    // Añadir células aleatorias para romper el estancamiento
    for (int i = 0; i < 20; i++) {
      int c = random(GRID_COLS);
      int f = random(GRID_ROWS);
      establecerCelula(currentGrid, c, f, true);
    }
    poblacionActual = contarPoblacion();
    Serial.println(F("Estancamiento roto - se añadieron células aleatorias"));
  }
}

// ═════════════════════════════════════════════════════════════
//  VERIFICACIÓN DE PROGRAMACIÓN HORARIA
// ═════════════════════════════════════════════════════════════
void verificarProgramacion() {
  if (!programacionActivada || !wifiConectado) return;
  
  time_t ahora = time(nullptr);
  struct tm* infoTiempo = localtime(&ahora);
  
  if (infoTiempo->tm_year < 100) return; // NTP aún no sincronizado
  
  int hora = infoTiempo->tm_hour;
  
  bool deberiaEstarEncendida;
  if (horaEncendido < horaApagado) {
    deberiaEstarEncendida = (hora >= horaEncendido && hora < horaApagado);
  } else {
    deberiaEstarEncendida = (hora >= horaEncendido || hora < horaApagado);
  }
  
  if (deberiaEstarEncendida && !pantallaEncendida) {
    pantallaEncendida = true;
    establecerRetroiluminacion(brillo);
    tft.fillScreen(themes[temaActual].muerta);
    dibujarCuadricula(true);
    dibujarBarraEstado();
  } else if (!deberiaEstarEncendida && pantallaEncendida) {
    pantallaEncendida = false;
    establecerRetroiluminacion(0);
  }
}

// ═════════════════════════════════════════════════════════════
//  PERSISTENCIA EN EEPROM
// ═════════════════════════════════════════════════════════════
void cargarConfiguracion() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) != EEPROM_MAGIC) {
    Serial.println(F("Sin configuración guardada, usando valores predeterminados"));
    return;
  }
  
  // Leer SSID
  char ssidBuf[33] = {0};
  for (int i = 0; i < 32; i++) ssidBuf[i] = EEPROM.read(EEPROM_ADDR_SSID + i);
  wifiSSID = String(ssidBuf);
  wifiSSID.trim();
  
  // Leer Contraseña
  char passBuf[65] = {0};
  for (int i = 0; i < 64; i++) passBuf[i] = EEPROM.read(EEPROM_ADDR_PASS + i);
  wifiPass = String(passBuf);
  wifiPass.trim();
  
  // Leer ajustes
  temaActual = EEPROM.read(EEPROM_ADDR_THEME) % NUM_THEMES;
  velocidadSim = EEPROM.read(EEPROM_ADDR_SPEED) | (EEPROM.read(EEPROM_ADDR_SPEED + 1) << 8);
  if (velocidadSim < 20 || velocidadSim > 2000) velocidadSim = 150;
  
  horaEncendido = EEPROM.read(EEPROM_ADDR_SCHON);
  horaApagado = EEPROM.read(EEPROM_ADDR_SCHOFF);
  programacionActivada = EEPROM.read(EEPROM_ADDR_SCHEN);
  brillo = EEPROM.read(EEPROM_ADDR_BRIGHT);
  if (brillo > 100) brillo = 100;
  
  // Leer estadísticas
  EEPROM.get(EEPROM_ADDR_GAMES, partidasTotales);
  EEPROM.get(EEPROM_ADDR_GENS, generacionesTotales);
  
  // Leer nombre del dispositivo
  char nameBuf[33] = {0};
  for (int i = 0; i < 32; i++) nameBuf[i] = EEPROM.read(EEPROM_ADDR_NAME + i);
  nombreDispositivo = String(nameBuf);
  nombreDispositivo.trim();
  if (nombreDispositivo.length() == 0) nombreDispositivo = "ConwayLife";
  
  // Leer zona horaria
  int8_t tzVal = (int8_t)EEPROM.read(EEPROM_ADDR_TZ);
  if (tzVal >= -12 && tzVal <= 14) {
    zonaHoraria = tzVal;
  } else {
    zonaHoraria = 1; // Por defecto CET
  }
  
  Serial.printf("Configuración cargada - SSID: %s, Tema: %d, Velocidad: %d, TZ: %+d\n", 
                wifiSSID.c_str(), temaActual, velocidadSim, zonaHoraria);
}

void guardarConfiguracion() {
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
  
  // Escribir SSID
  for (int i = 0; i < 32; i++) {
    EEPROM.write(EEPROM_ADDR_SSID + i, i < wifiSSID.length() ? wifiSSID[i] : 0);
  }
  
  // Escribir Contraseña
  for (int i = 0; i < 64; i++) {
    EEPROM.write(EEPROM_ADDR_PASS + i, i < wifiPass.length() ? wifiPass[i] : 0);
  }
  
  EEPROM.write(EEPROM_ADDR_THEME, temaActual);
  EEPROM.write(EEPROM_ADDR_SPEED, velocidadSim & 0xFF);
  EEPROM.write(EEPROM_ADDR_SPEED + 1, (velocidadSim >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDR_SCHON, horaEncendido);
  EEPROM.write(EEPROM_ADDR_SCHOFF, horaApagado);
  EEPROM.write(EEPROM_ADDR_SCHEN, programacionActivada);
  EEPROM.write(EEPROM_ADDR_BRIGHT, brillo);
  
  // Escribir nombre del dispositivo
  for (int i = 0; i < 32; i++) {
    EEPROM.write(EEPROM_ADDR_NAME + i, i < nombreDispositivo.length() ? nombreDispositivo[i] : 0);
  }
  
  // Escribir zona horaria
  EEPROM.write(EEPROM_ADDR_TZ, (uint8_t)zonaHoraria);
  
  EEPROM.commit();
  Serial.println(F("Configuración guardada"));
}

void guardarEstadisticas() {
  EEPROM.put(EEPROM_ADDR_GAMES, partidasTotales);
  EEPROM.put(EEPROM_ADDR_GENS, generacionesTotales);
  EEPROM.commit();
}
