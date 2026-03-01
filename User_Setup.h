// ═══════════════════════════════════════════════════════════════
//  User_Setup.h - TFT_eSPI Configuration
//  For: Wemos D1 Mini (ESP8266) + ILI9341 2.8" TFT Touch SPI
// ═══════════════════════════════════════════════════════════════
//
//  INSTRUCTIONS:
//  Copy this file to your Arduino libraries folder:
//    <Arduino>/libraries/TFT_eSPI/User_Setup.h
//  (Replace the existing file, make a backup first!)
//
//  Pin Mapping:
//    D1 (GPIO5)  -> CS        (TFT chip select)
//    D2 (GPIO4)  -> DC/RS     (Data/Command)
//    D3 (GPIO0)  -> T_CS      (Touch chip select)
//    D4 (GPIO2)  -> T_IRQ     (Touch interrupt, active LOW)
//    D5 (GPIO14) -> SCK       (SPI clock, shared)
//    D6 (GPIO12) -> MISO/SDO  (SPI data in, shared)
//    D7 (GPIO13) -> MOSI/SDI  (SPI data out, shared)
//    D8 (GPIO15) -> LED       (Backlight, PWM controlled in code)
//    RST -> RST pin or 3.3V via resistor
//
// ═══════════════════════════════════════════════════════════════

// ─── Driver ─────────────────────────────────────────────────
#define ILI9341_DRIVER

// ─── ESP8266 Pins (Wemos D1 Mini) ──────────────────────────
// Hardware SPI: MOSI=D7(GPIO13), CLK=D5(GPIO14), MISO=D6(GPIO12)
#define TFT_CS    5  // D1 (GPIO5)  - TFT Chip Select
#define TFT_DC    4  // D2 (GPIO4)  - Data/Command
#define TFT_RST  -1  // Not connected (use RST pin or tie to 3.3V)

// ─── Touch Screen Pins ─────────────────────────────────────
#define TOUCH_CS  0  // D3 (GPIO0) - Touch Chip Select

// ─── SPI Frequency ──────────────────────────────────────────
#define SPI_FREQUENCY       40000000   // 40MHz for display
#define SPI_READ_FREQUENCY  20000000   // 20MHz for reading
#define SPI_TOUCH_FREQUENCY  2500000   // 2.5MHz for touch

// ─── Font ───────────────────────────────────────────────────
#define LOAD_GLCD   // Font 1: Adafruit 8 pixel
#define LOAD_FONT2  // Font 2: 16 pixel
#define LOAD_FONT4  // Font 4: 26 pixel
#define LOAD_FONT6  // Font 6: 48 pixel numbers
#define LOAD_FONT7  // Font 7: 7 segment 48 pixel
#define LOAD_FONT8  // Font 8: 75 pixel
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT
