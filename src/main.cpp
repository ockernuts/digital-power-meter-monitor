#include <Arduino.h>
#include "sd_card.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "wifimanager.h"
#include "adafruit_gfx_displayer.h"
#include "print_displayer.h"
#include "timesync.h"
#include "hardware_and_pins.h"
#include "http_handlers.h"
#include "belgium_dsmr.h"
#include <ezLED.h>
#include "sdcardconfigpersistency.h"

// We wrapped the panic printer to be able to log to file...
void __real_panic_print_char(const char c);

void __wrap_panic_print_char(const char c) {
  // sdcard_panic_print_char(c);
  __real_panic_print_char(c);
}

WiFiClientSecure client;

AsyncWebServer server(80);

#if PROGRAM_MAIN_OUTPUT == OUTPUT_TYPE_TFT 
// We use a TFT 1.3 SCREEN without CS.
// It
Adafruit_ST7789 tft = Adafruit_ST7789(-1 /* NO CS */, TFT_DC, TFT_RST);
AdafruitGfxDisplayer displayer(tft);
#elif PROGRAM_MAIN_OUTPUT == OUTPUT_TYPE_SERIAL
PrintDisplayer displayer(Serial);
#elif PROGRAM_MAIN_OUTPUT == OUTPUT_TYPE_SERIAL1
PrintDisplayer displayer(Serial1);
#endif

// Choose Persistency layer for config (EEPROM or SDCard) -> something implementing IWifiConfigPersistency
SDCardConfigPersistency configPersistency; 

// The wifi manager will help us with initial IP setup. 
// It uses a "displayer" abstractor for its output
MyWifiManager wifiManager(server, displayer, configPersistency);

#define STARTUP_LED_STRENGHT 32

void setup() {
  Serial.begin(115200); // DEBUG OUT
  initLed();
  // Give 3 seconds so the monitorring pc can open the serial port for monitorring. 
  // In the meantime, animate the led. 
  setLedOn(/* not async */false, 3000, STARTUP_LED_STRENGHT);
   
  // Only after 3 seconds, show info. 
  Serial.printf("Digital meter monitor using board: %s\n", BOARD_NAME);
  Serial.printf("LED pin: %d\n", LED);

#ifdef TTGO_T8_ESP32_S2  
  pinMode(LED+1, OUTPUT);
  digitalWrite(LED+1, HIGH);
#endif  
  setLedOff(/*not async*/ false, 200, STARTUP_LED_STRENGHT);
  Serial.println("Ports used for SD Card communication:");
  Serial.print("MOSI: ");
  Serial.println(SD_CARD_MOSI);
  Serial.print("MISO: ");
  Serial.println(SD_CARD_MISO);
  Serial.print("SCK: ");
  Serial.println(SD_CARD_SCK);
  Serial.print("SS: ");
  Serial.println(SD_CARD_SS);  

  setLedOn(/*not async*/ false, 200, STARTUP_LED_STRENGHT);
  if (!setupSDCard(displayer)) {
    setLedOff(/*not async*/false, 200, STARTUP_LED_STRENGHT);
    delay(500);
    ESP.restart();
  }

  InitDsmrSerial();


  setLedOff(/*not async*/ false, 200, STARTUP_LED_STRENGHT);
  setLedOn(/*not async*/ false, 200, STARTUP_LED_STRENGHT);
  if (!wifiManager.Init()) {
    setLedOff(/*not async*/ false, 200, STARTUP_LED_STRENGHT);
    delay(500);
    // Init is not done... Wifi needs to be resetup through HTTP asynchronously.
    // So we return earlier to the "loop", which will check that we are not running in normal mode and eventually
    // Reset the ESP after it has been reconfigurred.
    // THis due to async handlers for HTTP being handled otside of loop. 
    AsyncElegantOTA.begin(&server);
    server.begin();
    return; // -> starts loop ! 
  }

  setLedOn(/*not async*/ false, 200, STARTUP_LED_STRENGHT);
  setupTimeSync(displayer);
  sdCardLogStartup();
  setLedOff(/*not async*/ false, 200, STARTUP_LED_STRENGHT);
  
  

#ifndef TTGO_T8_ESP32_S2
  // displayer.println("Telegram startup... ");
  // myBot.begin() ? displayer.println("OK") : displayer.println("NOK");
  // {
  //  char welcome_msg[64];
  //  snprintf(welcome_msg, sizeof(welcome_msg), "BOT @%s online\n/help all commands avalaible.", myBot.getBotName());
  //  myBot.sendTo(userid, welcome_msg);
  //}
  // delay(2000);
#endif

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  InitHttpHandlers(server);
  AsyncElegantOTA.begin(&server);
  server.begin();

  displayer.newPage("Verbruiksmonitor");

  // turns on RTS right away to get started.
  GetP1Reader().enable(true); 
  // from here, the led is on asynchronously, the loop() -> loopLed will do the neccesary animation. 
  setLedOn();
}


void loop() {
  // animate led, needs a fast loop. 
  loopLed();

  if (wifiManager.LoopWifiReconfigPending()) return;

  // We need a quick loop when results are still being read from the meter. 
  struct P1Data returned_data = {};
  if (!ProcessP1ReaderResults(returned_data)) {
    delay(10); // Not sure if this makes it really better, might be async stuff is better or worse this way, to evaluate...
    return; // -> not enough data yet-> early return to shorten loop -> wait for enough data 
  }

  // We have data !
  GetFixedQuarterPowerHistoryAccumulator().ProcessNewSample(returned_data);
  // TODO We better use a scheduler for this. (schedule Reader enable 100ms later) 
  setLedOff(/* async*/false);


  // At the end of the loop, we enable the meter output again (RTS), so we get a burst of new serial data to process. 
  GetP1Reader().enable(true);
  setLedOn();
}