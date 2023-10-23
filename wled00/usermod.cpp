
/*
 * This file allows you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * EEPROM bytes 2750+ are reserved for your custom use case. (if you extend #define EEPSIZE in const.h)
 * bytes 2400+ are currently ununsed, but might be used for future wled features
 */

/*
 * Pin 2 of the TTGO T-Display serves as the data line for the LED string.
 * Pin 35 is set up as the button pin in the platformio_overrides.ini file.
 * The button can be set up via the macros section in the web interface.
 * I use the button to cycle between presets.
 * The Pin 35 button is the one on the RIGHT side of the USB-C port on the board,
 * when the port is oriented downwards.  See readme.md file for photo.
 * The display is set up to turn off after 5 minutes, and turns on automatically 
 * when a change in the dipslayed info is detected (within a 5 second interval).
 */
 

//Use userVar0 and userVar1 (API calls &U0=,&U1=, uint16_t)

#include "wled.h"
#include <TFT_eSPI.h>
// #include <SPI.h>
#include "WiFi.h"
#include <Wire.h>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#ifndef TFT_MOSI
#define TFT_MOSI 19
#endif
#ifndef TFT_SCLK
#define TFT_SCLK 18
#endif
#ifndef TFT_CS
#define TFT_CS 5
#endif
#ifndef TFT_DC
#define TFT_DC 16
#endif
#ifndef TFT_RST
#define TFT_RST 23
#endif

#define TFT_BACKLIGHT_ON   HIGH

#define TFT_BL          4  // Display backlight control pin
#define ADC_EN          14  // Used for enabling battery voltage measurements - not used in this program
#define ADC_PIN             34

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

//gets called once at boot. Do all initialization that doesn't depend on network here
void userSetup() {
    Serial.println("Start");
    
    tft.init();
    tft.setRotation(3);  //Rotation here is set up for the text to be readable with the port on the left. Use 1 to flip.
    tft.fillScreen(TFT_RED);

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    // tft.setCursor(1, 10);
    tft.setTextDatum(MC_DATUM);
    // tft.setTextSize(3);
    // tft.print("Loading...");
    tft.drawString("Loading...",1,10,3);

    // if (TFT_BL > 0) { // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    //   pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
    //   digitalWrite(TFT_BL, HIGH); // Turn backlight on. 
    //   Serial.println("Setting TFT_BL : HIGH");
    // } else {
    //   digitalWrite(TFT_BL, LOW);
    //   Serial.println("Setting TFT_BL : LOW");
    // }
}

// gets called every time WiFi is (re-)connected. Initialize own network
// interfaces here
void userConnected() {
  Serial.println("Wifi Connected!");
  Serial.println(String("IP: " + WiFi.localIP().toString()));
}

// Next variables hold the previous known values to determine if redraw is
// required.
String knownSsid = "";
IPAddress knownIp;
uint8_t knownBrightness = 0;
uint8_t knownMode = 0;
uint8_t knownPalette = 0;
uint8_t tftcharwidth = 19;  // Number of chars that fit on screen with text size set to 2
uint8_t rtMode = 0;
IPAddress rtIp;
uint16_t estimatedMilliamps = 0;

long lastUpdateCheck = 0;
long lastRedraw = 0;
bool displayTurnedOff = false;
bool initialLoad = true;
// How often we are redrawing screen
#define USER_LOOP_REFRESH_RATE_MS 2000

boolean hasDataChanged() {
  if (initialLoad) {
    initialLoad = false;
    return true;
  }

  if (((apActive) ? String(apSSID) : WiFi.SSID()) != knownSsid) {
    return true;
  } else if (knownIp != (apActive ? IPAddress(4, 3, 2, 1) : WiFi.localIP())) {
    return true;
  } else if (knownBrightness != bri) {
    return true;
  } else if (knownMode != strip.getMainSegment().mode) {
    return true;
  } else if (knownPalette != strip.getMainSegment().palette) {
    return true;
  } else if (rtIp != realtimeIP) {
    return true;
  } else if (rtMode != realtimeMode) {
    return true;
  } else if (estimatedMilliamps != strip.currentMilliamps) {
    return true;
  }
  
  return false;
}

void setData() {
  // Update last known values.
  #if defined(ESP8266)
  knownSsid = apActive ? WiFi.softAPSSID() : WiFi.SSID();
  #else
  knownSsid = WiFi.SSID();
  #endif
  knownIp = apActive ? IPAddress(4, 3, 2, 1) : WiFi.localIP();
  knownBrightness = bri;
  knownMode = strip.getMainSegment().mode;
  knownPalette = strip.getMainSegment().palette;

  estimatedMilliamps = strip.currentMilliamps;

  rtIp = realtimeIP;
  rtMode = realtimeMode;
}

void displayBatteryPercentage() {
  int vref = 1100;
  uint16_t v = analogRead(ADC_PIN);
  float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
  int percentage = 0;
  if (battery_voltage >= 4.2) {
    percentage = 100;
  } else if (battery_voltage >= 4.15) {
    percentage = 95;
  } else if (battery_voltage >= 4.11) {
    percentage = 90;
  } else if (battery_voltage >= 4.08) {
    percentage = 85;
  } else if (battery_voltage >= 4.02) {
    percentage = 80;
  } else if (battery_voltage >= 3.98) {
    percentage = 75;
  } else if (battery_voltage >= 3.95) {
    percentage = 70;
  } else if (battery_voltage >= 3.91) {
    percentage = 65;
  } else if (battery_voltage >= 3.87) {
    percentage = 60;
  } else if (battery_voltage >= 3.85) {
    percentage = 55;
  } else if (battery_voltage >= 3.84) {
    percentage = 50;
  } else if (battery_voltage >= 3.82) {
    percentage = 45;
  } else if (battery_voltage >= 3.8) {
    percentage = 40;
  } else if (battery_voltage >= 3.79) {
    percentage = 35;
  } else if (battery_voltage >= 3.77) {
    percentage = 30;
  } else if (battery_voltage >= 3.75) {
    percentage = 25;
  } else if (battery_voltage >= 3.73) {
    percentage = 20;
  } else if (battery_voltage >= 3.71) {
    percentage = 15;
  } else if (battery_voltage >= 3.69) {
    percentage = 10;
  } else if (battery_voltage >= 3.61) {
    percentage = 5;
  } else {
    percentage = 0;
  }

  if (percentage > 30) {
    tft.setTextColor(TFT_GREEN);
  } else if (percentage > 10) {
    tft.setTextColor(TFT_YELLOW);
  } else {
    tft.setTextColor(TFT_RED);
  }
  
  // String voltage = String(battery_voltage) + "V";
  String value = String(percentage) + "%";
  tft.print(value);
  tft.setTextColor(TFT_WHITE);
}

void displayData() {
  // Clears Screen
  tft.fillScreen(TFT_BLACK);
  // Serial.println("Fill Screen Black");

  // First row with Wifi name
  tft.setTextSize(2);
  tft.setCursor(1, 1);
  tft.print(knownSsid);
  // Print `~` char to indicate that SSID is longer, than our dicplay
  if (knownSsid.length() > tftcharwidth)
  {
    tft.print("~");
  }

  // Second row with AP IP and Password or IP
  tft.setTextSize(2);
  tft.setCursor(1, 22);
  tft.print("IP: ");
  tft.print(knownIp);

  if (realtimeMode != REALTIME_MODE_E131) {
    tft.setTextSize(2);
    tft.setCursor(1,44);
    tft.print("Brightness: ");
    tft.print(((float(bri)/255)*100));
    tft.print("%");

    // Third row with mode name
    tft.setTextSize(2);
    tft.setCursor(1, 66);
    char lineBuffer[tftcharwidth+1];
    extractModeName(knownMode, JSON_mode_names, lineBuffer, tftcharwidth);
    tft.print(lineBuffer);

    // Fourth row with palette name
    tft.setCursor(1, 88);
    extractModeName(knownPalette, JSON_palette_names, lineBuffer, tftcharwidth);
    tft.print(lineBuffer);
  } else { 
    tft.setCursor(1, 44);
    tft.print("sACN Mode");
    tft.setCursor(1, 66);
    tft.print(MDNS_NAME);

    tft.setCursor(1, 88);
    tft.print(rtIp);
  }

  // Fifth row with estimated mA usage
  tft.setTextSize(2);
  tft.setCursor(1, 110);
  // Print estimated milliamp usage (must specify the LED type in LED prefs for this to be a reasonable estimate).
  if (strip.currentMilliamps < 1000) {
    tft.print(strip.currentMilliamps);
    tft.print(" mA");
  } else {
    tft.print((double)strip.currentMilliamps / 1000);
    tft.print(" A");
  }
  tft.print(" - ");

  displayBatteryPercentage();
}

void userLoop() {

  // Check if we time interval for redrawing passes.
  if (millis() - lastUpdateCheck < USER_LOOP_REFRESH_RATE_MS) {
    return;
  }
  lastUpdateCheck = millis();
  
  // Turn off display after 5 minutes with no Data change.
  if(!displayTurnedOff && millis() - lastRedraw > 5*60*1000) {
    digitalWrite(TFT_BL, LOW); // Turn backlight off. 
    displayTurnedOff = true;
    // Serial.println("Turning OFF Display");
  } 

  // Check if values which are shown on display changed from the last time.
  if (!hasDataChanged()) {
    Serial.println("No Data Changed");
    return;
  }
  Serial.println("Data Changed");

  // Serial.println("----------------------");
  lastRedraw = millis();

  // Turn on Display as Data has changed
  if (displayTurnedOff)
  {
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on.
    displayTurnedOff = false;
  }

  setData();

  displayData();  
}
